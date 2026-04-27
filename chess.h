#include <cctype>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
using namespace std;
using ull = unsigned long long;

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

  bool operator== (const Move& other) {
    return (
      startFile == other.startFile &&
      startRank == other.startRank &&
      endFile == other.endFile &&
      endRank == other.endRank
    );
  }
};

struct BoardState {
  ull pieces[2][7];
  ull occupancy[2];
  int currentTurn;

  bool operator== (const BoardState& other) {
    for (int c = 0; c < 2; c++) {
      if (occupancy[c] != other.occupancy[c]) return false;
      for (int i = 0; i < 7; i++) {
        if (pieces[c][i] != other.pieces[c][i]) return false;
      }
    }
    return currentTurn == other.currentTurn;
  }

  bool operator< (const BoardState& other) {
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
  ull pieces[2][7];  // [color][type] 0:White, 1:Black / 1:P, 2:N, 3:B, 4:R, 5:Q, 6:K
  ull occupancy[2];
  int currentTurn;  // 0: 백 차례, 1: 흑 차례
  vector<BoardState> history;  // undo()를 위한 히스토리 스택
  map<BoardState, int> vis;  // 포지션별 방문 횟수를 저장하는 map

  // 비트보드 업데이트 헬퍼 함수
  void updateOccupancy() {
    for (int c = 0; c < 2; c++) {
      occupancy[c] = 0;
      for (int t = 1; t <= 6; t++) occupancy[c] |= pieces[c][t];
    }
  }

  // 파일과 랭크(1~8)를 비트보드 시프트 인덱스(0~63)로 변환 (A열=7, H열=0 기준)
  int sqToIndex(int file, int rank) {
    int x = 8 - file;
    int y = rank - 1;
    return y * 8 + x;
  }

 public:
  Board() {
    memset(pieces, 0, sizeof(pieces));
    currentTurn = 0;
    updateOccupancy();
  }

  int turn() { return currentTurn; }

  vector<Move> getMoves() {
    vector<Move> moves;
    // TODO: 내부에 숨겨진 비트보드의 getKnightMoves, getSlidingMoves 등을
    // 호출하여 비트보드 결과를 구한 뒤, Move 객체로 변환하여 moves에 push_back
    // 합니다.
    return moves;
  }

  bool isCheck() {
    // TODO: 킹이 공격받고 있으면 true
    return false;
  }

  bool isCheckmate() { return (isCheck() && getMoves().empty()); }

  bool isStalemate() { return (isCheck() && !getMoves().empty()); }

  bool isDraw() {
    // 스테일메이트 판별
    if (isStalemate()) return true;

    // 3수동형 판별
    for (const auto& pa: vis) {
      if (pa.second >= 3) return true;
    }

    // 50수 규칙 판별: 50수 전과 현재를 비교하여 기물 수와 폰 위치가 같으면 됨
    // 기물부족 판별: 이걸 지금 해야 할까
    return false;
  }

  bool isValid(Move m) {
    for (Move i: getMoves()) {
      if (i == m) return true;
    }
    return false;
  }

  char getPiece(int file, int rank) {
    const char pieceChars[8] = ".PNBRQK";
    int bitIndex = sqToIndex(file, rank);
    ull bit = 1ULL << bitIndex;

    for (int c = 0; c < 2; c++) {
      for (int t = 1; t <= 6; t++) {
        if (pieces[c][t] & bit) return c == 0 ? pieceChars[t] : tolower(pieceChars[t]);
      }
    }
    return '.';
  }

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
    vis[state]++;

    // 2. 수 m을 적용하여 비트보드 조작
    // (출발지의 기물 비트를 끄고, 도착지에 비트를 켜며, 적 기물이 있다면
    // 끕니다)
    pair<int, int> start = m.getStart();
    pair<int, int> end = m.getEnd();

    // TODO: pieces 비트 조작 및 턴 넘기기 로직

    currentTurn ^= 1;  // 0과 1을 토글(턴 넘김)
    updateOccupancy();
  }

  void undo() {
    if (history.empty()) {
      throw runtime_error("Initial state, cannot undo");
    }

    // 스택에서 가장 최근 상태를 꺼내어 복구
    BoardState lastState = history.back();
    history.pop_back();
    vis[lastState]--;

    memcpy(pieces, lastState.pieces, sizeof(pieces));
    memcpy(occupancy, lastState.occupancy, sizeof(occupancy));
    currentTurn = lastState.currentTurn;
  }
};
