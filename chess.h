#include <cstring>
#include <iostream>
#include <vector>
using namespace std;
using ull = unsigned long long;

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

  pair<int, int> getStart() { return {startFile, startRank}; }
  pair<int, int> getEnd() { return {endFile, endRank}; }

  void setStart(int file, int rank) {
    startFile = file;
    startRank = rank;
  }

  void setEnd(int file, int rank) {
    endFile = file;
    endRank = rank;
  }
};

// ==========================================
// 보드의 이전 상태를 기억하기 위한 구조체 (undo용)
// ==========================================
struct BoardState {
  ull pieces[2][7];  // [color][type] 0:White, 1:Black / 1:P, 2:N, 3:B, 4:R,
                     // 5:Q, 6:K
  ull occupancy[2];
  ull NotMoved[2] = {}; //킹, 룩, 폰의 초기 위치에서 변화 여부를 저장
  int currentTurn;   // 0: 백 차례, 1: 흑 차례
  // 3수 동형, 50수 규칙을 위한 카운터나 해시값 등을 여기에 추가할 수 있습니다.
};

// ==========================================
// 2. Board 클래스: 하나의 체스판을 저장
// ==========================================
class Board {
 private:
  BoardState state;
  vector<BoardState> history;  // undo()를 위한 히스토리 스택

  // 비트보드 업데이트 헬퍼 함수
  // 수 하나당 업데이트 하는거면 전체 비트보드 탐색까진 필요 없어서 나중에 수를 둘 때마다 그 부분만 갱신하는 것도 고려
  
  void updateOccupancy() {
    for (int c = 0; c < 2; c++) {
      state.occupancy[c] = 0;
      for (int t = 1; t <= 6; t++) state.occupancy[c] |= state.pieces[c][t];
    }
  }

  // 파일과 랭크(1~8)를 비트보드 시프트 인덱스(0~63)로 변환 (A열=7, H열=0 기준)
  int sqToIndex(int file, int rank) {
    int x = 8 - file;
    int y = rank - 1;
    return y << 3 | x; //y * 8 + x
  }

  // 비트보드 시프트 인덱스(0~63)을 파일과 랭크(1~8)로 변환 (A열=7, H열=0 기준)
  pair<int, int> IndexTosq(int pos) { return {pos >> 3, pos | 7}; }

 public:
  // 생성자: 체스판 초기 세팅
  Board() {
    memset(pieces, 0, sizeof(pieces));
    currentTurn = 0;
    updateOccupancy();
  }

  // 현재 차례를 반환 (백 0, 흑 1)
  int turn() { return currentTurn; }

  // 현재 차례인 쪽이 둘 수 있는 수를 모두 반환
  vector<Move> getMoves() {
    vector<Move> moves;
    Move mv;
    if (turn() == 0) {
      // pawn
      for (int i = 1; i <= ULONG_LONG_MAX >> 8; i <<= 1){
        if(i & state.pieces[0][1]){
          mv.setStart(IndexTosq(i));
          mv.setEnd(IndexTosq(i << 3));
          moves.push_back(mv);
          if(notmoved & i){
            mv.setEnd(IndexTosq(i << 6));
            notmoved ^= i;
            moves.push_back(mv);
          }
          if(state.occupancy[1] & )
        }
      }
      //knight

    }

    // TODO: 내부에 숨겨진 비트보드의 getKnightMoves, getSlidingMoves 등을
    // 호출하여 비트보드 결과를 구한 뒤, Move 객체로 변환하여 moves에
    // push_back 합니다.
    return moves;
  }

  // 체크메이트 여부 반환
  bool isCheckmate() {
    // TODO: 킹이 공격받고 있고(Check), getMoves()의 결과가 비어있으면 true
    return false;
  }

  // 스테일메이트 여부 반환
  bool isStalemate() {
    // TODO: 킹이 공격받지 않고 있고, getMoves()의 결과가 비어있으면 true
    return false;
  }

  // 무승부 여부 반환 (기물 부족, 3수 동형, 50수 규칙)
  bool isDraw() {
    // TODO: 히스토리(history)를 탐색하여 3번 똑같은 배치가 나왔는지 확인하거나,
    // 양측 기물이 킹/나이트 등만 남아 체크메이트가 불가능한지 판별합니다.
    return false;
  }

  // 유효한 수인지 여부 반환
  bool isValid(Move m) {
    // TODO: getMoves() 목록에 해당 Move m이 존재하는지 확인합니다.
    return true;
  }

  // 특정 위치의 기물을 문자로 반환
  char getPiece(int file, int rank) {
    int bitIndex = sqToIndex(file, rank);
    ull bit = 1ULL << bitIndex;

    const char pieceChars[] = {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};

    for (int c = 0; c < 2; c++) {
      for (int t = 1; t <= 6; t++) {
        if (pieces[c][t] & bit) {
          // 백(0)은 대문자, 흑(1)은 소문자
          return c == 0 ? pieceChars[t] : tolower(pieceChars[t]);
        }
      }
    }
    return '.';  // 빈칸
  }

  // 수 m을 둔 상태로 보드를 갱신
  void update(Move m) {
    if (!isValid(m)) {
      throw runtime_error("Invalid Move Attempted");
    }

    // 1. 현재 상태를 백업하여 히스토리에 저장 (undo를 위해)
    BoardState state;
    memcpy(state.pieces, pieces, sizeof(pieces));
    memcpy(state.occupancy, occupancy, sizeof(occupancy));
    state.currentTurn = currentTurn;
    history.push_back(state);

    // 2. 수 m을 적용하여 비트보드 조작
    // (출발지의 기물 비트를 끄고, 도착지에 비트를 켜며, 적 기물이 있다면
    // 끕니다)
    pair<int, int> start = m.getStart();
    pair<int, int> end = m.getEnd();

    // TODO: pieces 비트 조작 및 턴 넘기기 로직

    currentTurn ^= 1;  // 0과 1을 토글(턴 넘김)
    updateOccupancy();
  }

  // 가장 최근 수를 되돌림
  void undo() {
    if (history.empty()) {
      throw runtime_error("Initial state, cannot undo");
    }

    // 스택에서 가장 최근 상태를 꺼내어 복구
    BoardState lastState = history.back();
    history.pop_back();

    memcpy(pieces, lastState.pieces, sizeof(pieces));
    memcpy(occupancy, lastState.occupancy, sizeof(occupancy));
    currentTurn = lastState.currentTurn;
  }
};
int main() { return 0; }
