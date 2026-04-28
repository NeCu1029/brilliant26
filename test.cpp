#include "chess.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

using namespace std;

string pieceName(char c) {
    switch (tolower(c)) {
        case 'p': return "폰";
        case 'n': return "나이트";
        case 'b': return "비숍";
        case 'r': return "룩";
        case 'q': return "퀸";
        case 'k': return "킹";
        default:  return "?";
    }
}

string toChessNotation(int file, int rank) {
    string s;
    s += (char)('a' + file - 1);
    s += (char)('0' + rank);
    return s;
}

void printBoard(Board& board) {
    for (int rank = 8; rank >= 1; rank--) {
        for (int file = 1; file <= 8; file++) {
            char p = board.getPiece(file, rank);
            cout << p << "  ";
        }
        cout << "\n";
    }
}

void printMoves(const vector<Move>& moves, Board& board) {
    vector<pair<int,int>> froms;
    for (auto& m : moves) {
        auto [sf, sr] = m.getStart();
        pair<int,int> key = {sf, sr};
        if (find(froms.begin(), froms.end(), key) == froms.end())
            froms.push_back(key);
    }

    int idx = 1;
    for (auto& [sf, sr] : froms) {
        char pc = board.getPiece(sf, sr);
        string color = (isupper(pc) ? "백" : "흑");
        cout << "  [" << color << " " << pieceName(pc) << " " << toChessNotation(sf, sr) << "] → ";
        bool first = true;
        for (auto& m : moves) {
            auto [msf, msr] = m.getStart();
            auto [mef, mer] = m.getEnd();
            if (msf == sf && msr == sr) {
                if (!first) cout << ", ";
                cout << idx << ": " << toChessNotation(mef, mer);
                first = false;
                idx++;
            }
        }
        cout << "\n";
    }
}

int main() {
    Board board;

    for (int f = 1; f <= 8; f++) board.setPiece(f, 2, 0, 1);
    board.setPiece(2, 1, 0, 2); board.setPiece(7, 1, 0, 2);
    board.setPiece(3, 1, 0, 3); board.setPiece(6, 1, 0, 3);
    board.setPiece(1, 1, 0, 4); board.setPiece(8, 1, 0, 4);
    board.setPiece(4, 1, 0, 5);
    board.setPiece(5, 1, 0, 6);
    for (int f = 1; f <= 8; f++) board.setPiece(f, 7, 1, 1);
    board.setPiece(2, 8, 1, 2); board.setPiece(7, 8, 1, 2);
    board.setPiece(3, 8, 1, 3); board.setPiece(6, 8, 1, 3);
    board.setPiece(1, 8, 1, 4); board.setPiece(8, 8, 1, 4);
    board.setPiece(4, 8, 1, 5);
    board.setPiece(5, 8, 1, 6);

    cout << "CLI 체스 프로그램\n";
    cout << "< 입력 형식 >\n";
    cout << "번호 입력: 해당 수 선택\n";
    cout << "undo: 한 수 되돌리기\n";
    cout << "quit: 종료\n\n";

    while (true) {
        printBoard(board);
        if (board.isCheckmate()) {
            string winner = (board.turn() == 0) ? "흑" : "백";
            cout << "체크메이트! " << winner << "의 승리!\n";
            break;
        }
        if (board.isDraw()) {
            cout << "무승부!\n";
            break;
        }

        string turnStr = (board.turn() == 0) ? "백(대문자)" : "흑(소문자)";
        cout << "── " << turnStr << " 차례 ──";
        if (board.isCheck()) cout << "  ⚠  체크!";
        cout << "\n\n";

        vector<Move> moves = board.getMoves();
        cout << "가능한 수 (" << moves.size() << "가지):\n";
        printMoves(moves, board);
        cout << "\n입력 > ";

        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        if (line == "quit" || line == "q") {
            cout << "종료합니다.\n";
            break;
        }
        if (line == "undo" || line == "u") {
            try {
                board.undo();
                cout << "한 수 되돌렸습니다.\n";
            } catch (...) {
                cout << "되돌릴 수 없습니다.\n";
            }
            continue;
        }

        bool isNumber = !line.empty() && all_of(line.begin(), line.end(), ::isdigit);
        if (isNumber) {
            int num = stoi(line);
            if (num < 1 || num > (int)moves.size()) {
                cout << "올바른 번호를 입력하세요 (1~" << moves.size() << ").\n";
                continue;
            }
            Move chosen = moves[num - 1];
            try {
                board.update(chosen);
                auto [ef, er] = chosen.getEnd();
                auto [sf, sr] = chosen.getStart();
                cout << toChessNotation(sf, sr) << " → " << toChessNotation(ef, er) << " 이동 완료.\n\n";
            } catch (const exception& e) {
                cout << "오류: " << e.what() << "\n";
            }
            continue;
        }
    }

    return 0;
}