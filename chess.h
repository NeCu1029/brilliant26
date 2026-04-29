#include <cctype>
#include <cstring>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
using namespace std;
using ull = unsigned long long;

// 비트보드 레이아웃:
//   인덱스 = (rank-1)*8 + (8-file)
//   즉, a1=7, b1=6, ..., h1=0, a2=15, ..., h8=56
//   FILE_A: file=1(A열) → x=7 → 비트 7,15,23,...,63
//   FILE_H: file=8(H열) → x=0 → 비트 0,8,16,...,56
const ull FILE_A = 0x8080808080808080ULL;
const ull FILE_H = 0x0101010101010101ULL;
const ull RANK_1 = 0x00000000000000FFULL;
const ull RANK_2 = 0x000000000000FF00ULL;
const ull RANK_7 = 0x00FF000000000000ULL;
const ull RANK_8 = 0xFF00000000000000ULL;

// ==========================================
// 1. Move 클래스: 하나의 수를 저장
// ==========================================
class Move {
 private:
  int startFile, startRank;
  int endFile, endRank;

 public:
  Move() : startFile(1), startRank(1), endFile(1), endRank(1) {}
  Move(int sf, int sr, int ef, int er)
      : startFile(sf), startRank(sr), endFile(ef), endRank(er) {}

  pair<int, int> getStart() const { return {startFile, startRank}; }
  pair<int, int> getEnd() const { return {endFile, endRank}; }

  void setStart(int file, int rank) {
    startFile = file;
    startRank = rank;
  }
  void setStart(pair<int,int> PII) {
    startFile = PII.first;
    startRank = PII.second;
  }

  void setEnd(int file, int rank) {
    endFile = file;
    endRank = rank;
  }
  void setEnd(pair<int,int> PII) {
    endFile = PII.first;
    endRank = PII.second;
  }

  bool operator==(const Move& other) const {
    return (startFile == other.startFile && startRank == other.startRank &&
            endFile == other.endFile && endRank == other.endRank);
  }
};

struct BoardState {
  ull pieces[2][7];  // [color][type] 0:White, 1:Black / 1:P,2:N,3:B,4:R,5:Q,6:K
  ull occupancy[2];
  // notmoved: 킹·룩·폰이 아직 한 번도 움직이지 않은 칸의 비트마스크
  ull notmoved[2] = {
      0xFF89ULL,              // 백: rank1의 킹(e1=sqToIndex(5,1))·룩(a1,h1) + rank2 폰 전체
      0x89FF000000000000ULL   // 흑: rank8의 킹·룩 + rank7 폰 전체
  };
  // 앙파상: 직전 턴에 폰이 두 칸 전진했다면, 그 폰이 지나친 칸(en passant 가능 칸)의 비트
  ull enPassantTarget = 0ULL;
  int currentTurn;    // 0: 백 차례, 1: 흑 차례

  bool operator==(const BoardState& other) const {
    for (int c = 0; c < 2; c++) {
      if (occupancy[c] != other.occupancy[c]) return false;
      for (int i = 0; i < 7; i++) {
        if (pieces[c][i] != other.pieces[c][i]) return false;
      }
    }
    return currentTurn == other.currentTurn;
  }

  bool operator<(const BoardState& other) const {
    for (int c = 0; c < 2; c++) {
      if (occupancy[c] < other.occupancy[c]) return true;
      if (occupancy[c] > other.occupancy[c]) return false;
      for (int i = 0; i < 7; i++) {
        if (pieces[c][i] < other.pieces[c][i]) return true;
        if (pieces[c][i] > other.pieces[c][i]) return false;
      }
    }
    return currentTurn < other.currentTurn;
  }
};

class Board {
 private:
  BoardState state;
  vector<BoardState> history;
  map<BoardState, int> vis;

  void updateOccupancy() {
    for (int c = 0; c < 2; c++) {
      state.occupancy[c] = 0;
      for (int t = 1; t <= 6; t++) state.occupancy[c] |= state.pieces[c][t];
    }
  }

  // (file, rank) → 비트 인덱스.  file 1=A, rank 1..8
  int sqToIndex(int file, int rank) const {
    int x = 8 - file;   // A열→7, H열→0
    int y = rank - 1;   // rank1→0, rank8→7
    return y * 8 + x;
  }

  // 비트 인덱스 → (file, rank)
  pair<int, int> IndexTosq(int pos) const {
    int y = pos / 8;       // 0‥7
    int x = pos % 8;       // 0(H열)‥7(A열)
    return {8 - x, y + 1};
  }

  // 비트 하나에서 인덱스를 뽑아 Move 종점으로 변환
  Move makeMove(int fromIdx, int toIdx) const {
    auto [sf, sr] = IndexTosq(fromIdx);
    auto [ef, er] = IndexTosq(toIdx);
    return Move(sf, sr, ef, er);
  }

  // ──────────────────────────────────────────
  // 공격 비트보드 생성 (isAttackedBy에서 사용)
  // ──────────────────────────────────────────

  // 나이트 공격 마스크 (인덱스 기준)
  ull knightAttacks(int idx) const {
    ull b = 1ULL << idx;
    ull noA  = b & ~FILE_A;
    ull noH  = b & ~FILE_H;
    // noAB: A열(x=7) 또는 B열(x=6)에 있지 않은 경우 → 왼쪽으로 2칸 이동 가능
    ull noAB = b & ~FILE_A & ~(FILE_A >> 1);
    // noGH: G열(x=1) 또는 H열(x=0)에 있지 않은 경우 → 오른쪽으로 2칸 이동 가능
    ull noGH = b & ~FILE_H & ~(FILE_H << 1);

    return (noAB << 10) | (noAB >> 6) |   // +2rank ±1file
           (noGH <<  6) | (noGH >> 10) |  // +2rank ±1file 반대쪽
           (noA  << 17) | (noA  >> 15) |  // +1rank ±2file
           (noH  << 15) | (noH  >> 17);
  }

  // 킹 공격 마스크
  ull kingAttacks(int idx) const {
    ull b = 1ULL << idx;
    ull noA = b & ~FILE_A;
    ull noH = b & ~FILE_H;
    return (b << 8) | (b >> 8) |
           (noA << 9) | (noA << 1) | (noA >> 7) |
           (noH << 7) | (noH >> 1) | (noH >> 9);
  }

  // 슬라이딩 기물 공격 마스크 (ray casting)
  ull rookAttacks(int idx, ull occ) const {
    ull attacks = 0;
    // 북 (+8)
    for (int s = idx + 8; s < 64; s += 8) {
      attacks |= 1ULL << s;
      if (occ >> s & 1) break;
    }
    // 남 (-8)
    for (int s = idx - 8; s >= 0; s -= 8) {
      attacks |= 1ULL << s;
      if (occ >> s & 1) break;
    }
    // 동 (-1, x 감소 → file 증가)
    for (int s = idx - 1; s >= 0 && s / 8 == idx / 8; s--) {
      attacks |= 1ULL << s;
      if (occ >> s & 1) break;
    }
    // 서 (+1, x 증가 → file 감소)
    for (int s = idx + 1; s < 64 && s / 8 == idx / 8; s++) {
      attacks |= 1ULL << s;
      if (occ >> s & 1) break;
    }
    return attacks;
  }

  ull bishopAttacks(int idx, ull occ) const {
    ull attacks = 0;
    int r = idx / 8, f = idx % 8;
    // 대각선 네 방향
    for (int dr : {1, -1}) for (int df : {1, -1}) {
      int nr = r + dr, nf = f + df;
      while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
        int s = nr * 8 + nf;
        attacks |= 1ULL << s;
        if (occ >> s & 1) break;
        nr += dr; nf += df;
      }
    }
    return attacks;
  }

  ull queenAttacks(int idx, ull occ) const {
    return rookAttacks(idx, occ) | bishopAttacks(idx, occ);
  }

  // ──────────────────────────────────────────
  // 특정 칸이 color의 기물에게 공격받는가
  // ──────────────────────────────────────────
  bool isAttackedBy(int targetIdx, int color, const BoardState& s) const {
    ull occ = s.occupancy[0] | s.occupancy[1];

    // 폰 공격
    if (color == 0) {  // 백 폰: 위쪽(+8)에서 대각으로 공격
      ull pawnAtk = 0;
      ull t = 1ULL << targetIdx;
      if (!(t & FILE_A)) pawnAtk |= t >> 9;  // 백 폰이 좌상단에서 공격 (x+1, r+1)
      if (!(t & FILE_H)) pawnAtk |= t >> 7;  // 백 폰이 우상단에서 공격 (x-1, r+1)
      if (pawnAtk & s.pieces[color][1]) return true;
    } else {  // 흑 폰: 아래쪽(-8)에서 대각으로 공격
      ull pawnAtk = 0;
      ull t = 1ULL << targetIdx;
      if (!(t & FILE_H)) pawnAtk |= t << 7;
      if (!(t & FILE_A)) pawnAtk |= t << 9;
      if (pawnAtk & s.pieces[color][1]) return true;
    }

    // 나이트
    if (knightAttacks(targetIdx) & s.pieces[color][2]) return true;
    // 비숍/퀸
    if (bishopAttacks(targetIdx, occ) & (s.pieces[color][3] | s.pieces[color][5])) return true;
    // 룩/퀸
    if (rookAttacks(targetIdx, occ) & (s.pieces[color][4] | s.pieces[color][5])) return true;
    // 킹
    if (kingAttacks(targetIdx) & s.pieces[color][6]) return true;

    return false;
  }

  // 현재 state에서 color 킹이 체크인가
  bool isKingInCheck(int color, const BoardState& s) const {
    ull kingBit = s.pieces[color][6];
    if (!kingBit) return false;
    int kingIdx = __builtin_ctzll(kingBit);
    return isAttackedBy(kingIdx, 1 - color, s);
  }

  // ──────────────────────────────────────────────────────
  // 비트보드에 수 적용 후 자기 킹이 체크에 걸리는지 검사
  // (의사수 필터링)
  // ──────────────────────────────────────────────────────
  bool leavesKingInCheck(Move m, bool isEnPassant = false, bool isCastle = false) const {
    // 상태 복사 후 수를 적용하여 검사
    BoardState tmp = state;
    int color = tmp.currentTurn;
    int opp   = 1 - color;

    auto [sf, sr] = m.getStart();
    auto [ef, er] = m.getEnd();
    int fromIdx = sqToIndex(sf, sr);
    int toIdx   = sqToIndex(ef, er);
    ull fromBit = 1ULL << fromIdx;
    ull toBit   = 1ULL << toIdx;

    // 이동 기물 찾기
    int pieceType = -1;
    for (int t = 1; t <= 6; t++) {
      if (tmp.pieces[color][t] & fromBit) { pieceType = t; break; }
    }
    if (pieceType < 0) return true;  // 기물 없음 → 잘못된 수

    // 출발지 비트 제거
    tmp.pieces[color][pieceType] &= ~fromBit;

    // 적 기물 제거 (일반 캡처)
    for (int t = 1; t <= 6; t++) tmp.pieces[opp][t] &= ~toBit;

    // 도착지 비트 세팅
    tmp.pieces[color][pieceType] |= toBit;

    // 앙파상: 캡처된 폰 제거
    if (isEnPassant) {
      // 백이 앙파상하면 toIdx-8이 흑 폰, 흑이면 toIdx+8이 백 폰
      int capturedIdx = (color == 0) ? toIdx - 8 : toIdx + 8;
      tmp.pieces[opp][1] &= ~(1ULL << capturedIdx);
    }

    // 캐슬링: 룩도 이동
    if (isCastle) {
      if (ef > sf) {  // 킹사이드
        int rookFrom = sqToIndex(8, sr);  // H열
        int rookTo   = sqToIndex(6, sr);  // F열
        tmp.pieces[color][4] &= ~(1ULL << rookFrom);
        tmp.pieces[color][4] |=  (1ULL << rookTo);
      } else {        // 퀸사이드
        int rookFrom = sqToIndex(1, sr);  // A열
        int rookTo   = sqToIndex(4, sr);  // D열
        tmp.pieces[color][4] &= ~(1ULL << rookFrom);
        tmp.pieces[color][4] |=  (1ULL << rookTo);
      }
    }

    // occupancy 갱신
    for (int c = 0; c < 2; c++) {
      tmp.occupancy[c] = 0;
      for (int t = 1; t <= 6; t++) tmp.occupancy[c] |= tmp.pieces[c][t];
    }

    return isKingInCheck(color, tmp);
  }

  // ──────────────────────────────────────────────────────
  // 수 생성 헬퍼: 후보 수를 검증(킹 체크 필터)하여 추가
  // ──────────────────────────────────────────────────────
  void addIfLegal(vector<Move>& moves, Move m,
                  bool isEnPassant = false, bool isCastle = false) const {
    if (!leavesKingInCheck(m, isEnPassant, isCastle))
      moves.push_back(m);
  }

  // ──────────────────────────────────────────
  // 개별 기물 수 생성
  // ──────────────────────────────────────────

  void generatePawnMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    int opp   = 1 - color;
    ull pawns = state.pieces[color][1];
    ull occ   = state.occupancy[0] | state.occupancy[1];

    while (pawns) {
      int idx = __builtin_ctzll(pawns);
      pawns &= pawns - 1;
      ull bit = 1ULL << idx;
      auto [file, rank] = IndexTosq(idx);

      if (color == 0) {  // ─── 백 폰 (rank 증가 방향: +8)
        // 한 칸 전진
        int fwd = idx + 8;
        if (fwd < 64 && !((occ >> fwd) & 1)) {
          addIfLegal(moves, makeMove(idx, fwd));
          // 두 칸 전진 (rank2에서, 경로 비어있을 때)
          if (rank == 2) {
            int fwd2 = idx + 16;
            if (!((occ >> fwd2) & 1))
              addIfLegal(moves, makeMove(idx, fwd2));
          }
        }
        // 대각 캡처 (좌상단: x+1 → file-1, 우상단: x-1 → file+1)
        if (!(bit & FILE_A)) {  // A열이 아님 → 왼쪽(file-1, rank+1) 공격 가능
          int cap = idx + 9;    // rank+1, x+1
          if ((state.occupancy[opp] >> cap) & 1)
            addIfLegal(moves, makeMove(idx, cap));
          // 앙파상
          if (state.enPassantTarget && ((state.enPassantTarget >> cap) & 1))
            addIfLegal(moves, makeMove(idx, cap), true);
        }
        if (!(bit & FILE_H)) {  // H열이 아님 → 오른쪽(file+1, rank+1) 공격 가능
          int cap = idx + 7;    // rank+1, x-1
          if ((state.occupancy[opp] >> cap) & 1)
            addIfLegal(moves, makeMove(idx, cap));
          // 앙파상
          if (state.enPassantTarget && ((state.enPassantTarget >> cap) & 1))
            addIfLegal(moves, makeMove(idx, cap), true);
        }
      } else {  // ─── 흑 폰 (rank 감소 방향: -8)
        // 한 칸 전진
        int fwd = idx - 8;
        if (fwd >= 0 && !((occ >> fwd) & 1)) {
          addIfLegal(moves, makeMove(idx, fwd));
          // 두 칸 전진
          if (rank == 7) {
            int fwd2 = idx - 16;
            if (!((occ >> fwd2) & 1))
              addIfLegal(moves, makeMove(idx, fwd2));
          }
        }
        // 대각 캡처
        if (!(bit & FILE_A)) {
          int cap = idx - 7;
          if (cap >= 0 && (state.occupancy[opp] >> cap) & 1)
            addIfLegal(moves, makeMove(idx, cap));
          if (state.enPassantTarget && cap >= 0 && ((state.enPassantTarget >> cap) & 1))
            addIfLegal(moves, makeMove(idx, cap), true);
        }
        if (!(bit & FILE_H)) {
          int cap = idx - 9;
          if (cap >= 0 && (state.occupancy[opp] >> cap) & 1)
            addIfLegal(moves, makeMove(idx, cap));
          if (state.enPassantTarget && cap >= 0 && ((state.enPassantTarget >> cap) & 1))
            addIfLegal(moves, makeMove(idx, cap), true);
        }
      }
    }
  }

  void generateKnightMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    ull knights = state.pieces[color][2];
    ull myOcc   = state.occupancy[color];

    while (knights) {
      int idx = __builtin_ctzll(knights);
      knights &= knights - 1;
      ull atk = knightAttacks(idx) & ~myOcc;
      while (atk) {
        int to = __builtin_ctzll(atk);
        atk &= atk - 1;
        addIfLegal(moves, makeMove(idx, to));
      }
    }
  }

  void generateBishopMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    ull bishops = state.pieces[color][3];
    ull occ     = state.occupancy[0] | state.occupancy[1];
    ull myOcc   = state.occupancy[color];

    while (bishops) {
      int idx = __builtin_ctzll(bishops);
      bishops &= bishops - 1;
      ull atk = bishopAttacks(idx, occ) & ~myOcc;
      while (atk) {
        int to = __builtin_ctzll(atk);
        atk &= atk - 1;
        addIfLegal(moves, makeMove(idx, to));
      }
    }
  }

  void generateRookMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    ull rooks = state.pieces[color][4];
    ull occ   = state.occupancy[0] | state.occupancy[1];
    ull myOcc = state.occupancy[color];

    while (rooks) {
      int idx = __builtin_ctzll(rooks);
      rooks &= rooks - 1;
      ull atk = rookAttacks(idx, occ) & ~myOcc;
      while (atk) {
        int to = __builtin_ctzll(atk);
        atk &= atk - 1;
        addIfLegal(moves, makeMove(idx, to));
      }
    }
  }

  void generateQueenMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    ull queens = state.pieces[color][5];
    ull occ    = state.occupancy[0] | state.occupancy[1];
    ull myOcc  = state.occupancy[color];

    while (queens) {
      int idx = __builtin_ctzll(queens);
      queens &= queens - 1;
      ull atk = queenAttacks(idx, occ) & ~myOcc;
      while (atk) {
        int to = __builtin_ctzll(atk);
        atk &= atk - 1;
        addIfLegal(moves, makeMove(idx, to));
      }
    }
  }

  void generateKingMoves(vector<Move>& moves) const {
    int color = state.currentTurn;
    ull kingBit = state.pieces[color][6];
    if (!kingBit) return;
    int idx = __builtin_ctzll(kingBit);
    ull atk = kingAttacks(idx) & ~state.occupancy[color];
    ull occ = state.occupancy[0] | state.occupancy[1];

    while (atk) {
      int to = __builtin_ctzll(atk);
      atk &= atk - 1;
      addIfLegal(moves, makeMove(idx, to));
    }

    // ──────────────────────────────────────
    // 캐슬링
    // 조건:
    //  1. 킹과 해당 룩이 한 번도 움직이지 않았어야 함 (notmoved 확인)
    //  2. 킹과 룩 사이가 비어 있어야 함
    //  3. 킹이 현재 체크 상태가 아니어야 함
    //  4. 킹이 지나치는 칸과 도착 칸이 공격받으면 안 됨
    // ──────────────────────────────────────
    int rank = (color == 0) ? 1 : 8;
    int kingFile = 5;  // e열
    int kingIdx  = sqToIndex(kingFile, rank);

    // 킹이 체크 중이면 캐슬링 불가
    if (isKingInCheck(color, state)) return;

    // 킹이 제자리에 있어야 함 (notmoved 확인)
    ull kingNotMovedBit = 1ULL << kingIdx;
    if (!(state.notmoved[color] & kingNotMovedBit)) return;

    // 킹사이드 캐슬링 (e→g, 룩 h→f)
    {
      int rookIdx = sqToIndex(8, rank);  // h열
      ull rookNotMovedBit = 1ULL << rookIdx;
      if ((state.notmoved[color] & rookNotMovedBit) &&   // 룩 미이동
          !(occ & (1ULL << sqToIndex(6, rank))) &&        // f열 비어있음
          !(occ & (1ULL << sqToIndex(7, rank)))) {        // g열 비어있음 (실제는 없음, f/g만 확인)
        // 킹이 f열, g열을 지나치는 동안 공격받으면 안 됨
        bool safe = !isAttackedBy(sqToIndex(6, rank), 1 - color, state) &&
                    !isAttackedBy(sqToIndex(7, rank), 1 - color, state);
        if (safe) {
          Move castleMove(kingFile, rank, 7, rank);  // e→g
          addIfLegal(moves, castleMove, false, true);
        }
      }
    }

    // 퀸사이드 캐슬링 (e→c, 룩 a→d)
    {
      int rookIdx = sqToIndex(1, rank);  // a열
      ull rookNotMovedBit = 1ULL << rookIdx;
      if ((state.notmoved[color] & rookNotMovedBit) &&
          !(occ & (1ULL << sqToIndex(2, rank))) &&   // b열
          !(occ & (1ULL << sqToIndex(3, rank))) &&   // c열
          !(occ & (1ULL << sqToIndex(4, rank)))) {   // d열
        // 킹이 c열, d열을 지나치는 동안 공격받으면 안 됨
        bool safe = !isAttackedBy(sqToIndex(4, rank), 1 - color, state) &&
                    !isAttackedBy(sqToIndex(3, rank), 1 - color, state);
        if (safe) {
          Move castleMove(kingFile, rank, 3, rank);  // e→c
          addIfLegal(moves, castleMove, false, true);
        }
      }
    }
  }

 public:
  Board() {
    memset(state.pieces, 0, sizeof(state.pieces));
    state.currentTurn = 0;
    state.enPassantTarget = 0;
    updateOccupancy();
  }

  int turn() const { return state.currentTurn; }

  vector<Move> getMoves() const {
    vector<Move> moves;
    generatePawnMoves(moves);
    generateKnightMoves(moves);
    generateBishopMoves(moves);
    generateRookMoves(moves);
    generateQueenMoves(moves);
    generateKingMoves(moves);
    return moves;
  }

  bool isCheck() const {
    return isKingInCheck(state.currentTurn, state);
  }

  bool isCheckmate() const { return isCheck() && getMoves().empty(); }

  bool isStalemate() const { return !isCheck() && getMoves().empty(); }

  bool isDraw() const {
    if (isStalemate()) return true;
    for (const auto& pa: vis) {
      if (pa.second >= 3) return true;
    }

    int s = history.size(), c1 = 0, c2 = 0;
    if (s > 100) {
      for (int i = 0; i < 64; i++) {
        if (history[s - 1].occupancy[0] & (1ULL << i)) c1++;
        if (history[s - 1].occupancy[1] & (1ULL << i)) c1++;
        if (history[s - 101].occupancy[0] & (1ULL << i)) c2++;
        if (history[s - 101].occupancy[0] & (1ULL << i)) c2++;
      }
      if (
        c1 == c2 &&
        history[s - 1].pieces[0][1] == history[s - 101].pieces[0][1] &&
        history[s - 1].pieces[1][1] == history[s - 101].pieces[1][1]
      ) return true;
    }

    // 기물 부족
    return false;
  }

  bool isValid(Move m) const {
    for (Move i: getMoves()) {
      if (i == m) return true;
    }
    return false;
  }

  char getPiece(int file, int rank) const {
    const char pieceChars[8] = ".PNBRQK";
    int bitIndex = sqToIndex(file, rank);
    ull bit = 1ULL << bitIndex;
    for (int c = 0; c < 2; c++) {
      for (int t = 1; t <= 6; t++) {
        if (state.pieces[c][t] & bit) {
          return c == 0 ? pieceChars[t] : tolower(pieceChars[t]);
        }
      }
    }
    return '.';
  }

  void update(Move m) {
    if (!isValid(m)) throw runtime_error("Invalid Move Attempted");

    history.push_back(state);
    vis[state]++;

    int color = state.currentTurn;
    int opp   = 1 - color;

    auto [sf, sr] = m.getStart();
    auto [ef, er] = m.getEnd();
    int fromIdx = sqToIndex(sf, sr);
    int toIdx   = sqToIndex(ef, er);
    ull fromBit = 1ULL << fromIdx;
    ull toBit   = 1ULL << toIdx;

    // 이동 기물 찾기
    int pieceType = -1;
    for (int t = 1; t <= 6; t++) {
      if (state.pieces[color][t] & fromBit) { pieceType = t; break; }
    }

    // 앙파상 캡처 판별: 폰이 대각으로 이동했는데 도착칸에 적이 없는 경우
    bool isEnPassant = false;
    if (pieceType == 1 && sf != ef && !(state.occupancy[opp] & toBit)) {
      isEnPassant = true;
    }

    // 캐슬링 판별: 킹이 두 칸 이동
    bool isCastle = (pieceType == 6 && abs(ef - sf) == 2);

    // notmoved 갱신 (킹·룩·폰이 처음 움직이면 해당 비트 해제)
    state.notmoved[color] &= ~fromBit;

    // 출발지 비트 제거
    state.pieces[color][pieceType] &= ~fromBit;

    // 적 기물 캡처 (일반)
    for (int t = 1; t <= 6; t++) state.pieces[opp][t] &= ~toBit;

    // 도착지 세팅
    state.pieces[color][pieceType] |= toBit;

    // 앙파상: 잡힌 폰 제거
    if (isEnPassant) {
      int capturedIdx = (color == 0) ? toIdx - 8 : toIdx + 8;
      ull capBit = 1ULL << capturedIdx;
      state.pieces[opp][1] &= ~capBit;
      state.notmoved[opp]   &= ~capBit;
    }

    // 캐슬링: 룩 이동
    if (isCastle) {
      if (ef > sf) {  // 킹사이드
        int rookFrom = sqToIndex(8, sr);
        int rookTo   = sqToIndex(6, sr);
        state.pieces[color][4] &= ~(1ULL << rookFrom);
        state.pieces[color][4] |=  (1ULL << rookTo);
        state.notmoved[color]   &= ~(1ULL << rookFrom);
      } else {        // 퀸사이드
        int rookFrom = sqToIndex(1, sr);
        int rookTo   = sqToIndex(4, sr);
        state.pieces[color][4] &= ~(1ULL << rookFrom);
        state.pieces[color][4] |=  (1ULL << rookTo);
        state.notmoved[color]   &= ~(1ULL << rookFrom);
      }
    }

    // 앙파상 타깃 갱신: 폰이 두 칸 전진하면 지나친 칸을 기록
    state.enPassantTarget = 0;
    if (pieceType == 1 && abs(er - sr) == 2) {
      int epIdx = (color == 0) ? fromIdx + 8 : fromIdx - 8;
      state.enPassantTarget = 1ULL << epIdx;
    }

    state.currentTurn ^= 1;
    updateOccupancy();
  }

  void undo() {
    if (history.empty()) throw runtime_error("Initial state, cannot undo");
    BoardState lastState = history.back();
    history.pop_back();
    vis[lastState]--;
    state = lastState;
  }

  // 기물 배치 (테스트/초기화용)
  void setPiece(int file, int rank, int color, int type) {
    int idx = sqToIndex(file, rank);
    state.pieces[color][type] |= 1ULL << idx;
    updateOccupancy();
  }
};