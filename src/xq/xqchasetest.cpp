//
//  xqchasetest.cpp
//  FelicityEgtb-xq
//
//  Created by Nguyen Pham on 20/8/2024.
//  Copyright © 2024 Softgaroo. All rights reserved.
//

#include <map>
#include <sstream>
#include <iostream>
#include <assert.h>

#include <fstream>

#include "xqchasejudge.h"
#include "../base/funcs.h"
#include "../base/types.h"

#ifdef _FELICITY_XQ_
using namespace bslib;

//namespace bslib {

extern const char* reasonStrings[];
extern const std::string resultStrings[];

GameResultType string2ResultType(const std::string& s)
{
    for(int i = 0; !resultStrings[i].empty(); i++) {
        if (resultStrings[i] == s) {
            return static_cast<GameResultType>(i);
        }
    }
    return GameResultType::unknown;
}


std::vector<std::string> readTextFileToArray(const std::string& path)
{
//#if (defined (_WIN32) || defined (_WIN64))
//    std::ifstream inFile(std::filesystem::u8path(path.c_str()));
//#else
//    std::ifstream inFile(path);
//#endif

    std::ifstream inFile(path);
    
    std::string line;
    std::vector<std::string> vec;

    try {
        while (getline(inFile, line)) {
            vec.push_back(line);
        }
    } catch(const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return vec;
}


//bool _fromMoveList(XqBoard& board, const std::string& moveText,
//                             const std::vector<std::string>& moveStringVec,
//                             const std::map<size_t, std::string>& commentMap,
//                             const std::map<size_t, std::string>& eSymMap
//                              )
//{
//    auto hit = false;
//    auto parseMoveErrorCnt = 0;
//
////    auto lastLevel = 0, lastGroupIdx = 0;
////
////    /// Confirm rank09 or rank110
////    if (!moveStringVec.empty()) {
////        auto confirmed = false;
////        for(auto && s : moveStringVec) {
////            if (s.moveString.find('0') != std::string::npos) {
////                confirmed = true;
////                mf.rankFrom1 = s.moveString.find("10") != std::string::npos;
////                break;
////            }
////        }
////
////        if (!confirmed) {
////            auto ss = moveStringVec.front().moveString;
////            Move move = Move::illegalMove;
////
////            move = moveFromString_san(ss, mf);
////            if (!isValid(move)) {
////                move = moveFromString_xq(ss, mf);
////            }
////            if (!isValid(move)) {
////                move = moveFromString_coordinate(ss, mf);
////            }
////
////            auto ok = isValid(move);
////            if (ok) {
////                auto piece = _getPiece(move.from);
////                auto cap = _getPiece(move.dest);
////                ok = piece.side == side && piece.side != cap.side;
////            }
////            if (!ok) {
////                mf.rankFrom1 = !mf.rankFrom1;
////            }
////        }
////    }
//
//    for(size_t i = 0, coordinateCnt = 0; i < moveStringVec.size(); i++) {
//        auto ss = moveStringVec.at(i); assert(!ss.empty());
//
//        /// Ignore string with $ such as $10 (from Lichess games)
//        if (ss.at(0) == '$' && ss.length() > 1 && std::isdigit(ss.at(1))) {
//            continue;
//        }
//
//
////        Move move = Move::illegalMove;
//        auto move = moveFromString_xq(ss);
//
////        if (notation == Notation::san) {
////            move = moveFromString_san(ss, mf);
////        }
////        if (!board.isValid(move)) { // notation >= Notation::traditional_en) {
////            move = moveFromString_xq(ss);
//////            assert(move == Move::illegalMove || isValid(move));
////        }
////        if (!isValid(move)) {
////            move = moveFromString_coordinate(ss, mf);
////
////            if (notation == Notation::san && move != Move::illegalMove) {
////                coordinateCnt++;
////            }
////        }
//
//        if (!isValid(move)) {// move == Move::illegalMove) {
//            parseMoveErrorCnt++;
////            std::cerr << "Error: can't parse this move string" << ss << std::endl;
////            printOut("illegalMove");
//            return false;
//        }
//
//        /// Parse comment before making move for parsing pv
//        auto hasComment = false;
//
//        Hist tmphist;
////        if (!commentMap.empty()) {
////            auto it = commentMap.find(i + 1);
////            if (it != commentMap.end()) {
////                if (flag & ParseMoveListFlag_parseComment) {
////                    _parseComment(it->second, tmphist);
////                } else {
////                    if (!tmphist.comment.empty()) {
////                        tmphist.comment + ", ";
////                    }
////                    tmphist.comment += it->second;
////                }
////                hasComment = true;
////            }
////        }
////
////        std::string fenString;
////        if (flag & ParseMoveListFlag_create_fen) {
////            fenString = getFen(false);
////        }
//
//        /// Making move
//        if (!board.checkMake(move.from, move.dest, move.promotion)) {
//            parseMoveErrorCnt++;
//            return false;
//        }
//
////        auto lastHist = &histList.back();
////
////        assert(!histList.empty());
////        if (hasComment) {
////            histList.back().comment = tmphist.comment;
////            histList.back().esVec = tmphist.esVec;
////        }
////
////        if (!eSymMap.empty()) {
////            auto it = eSymMap.find(histList.size());
////            if (it != eSymMap.end()) {
////                histList.back().mes = string2MoveEvaluationSymbol(it->second);
////            }
////        }
////
////        if (flag & ParseMoveListFlag_create_fen) {
////            histList.back().fenString = fenString;
////        }
//    }
//
//    return true;
//}

int xqCoordinateStringToPos(const std::string& str)
{
    auto colChr = str[0], rowChr = str[1];
    if (colChr >= 'a' && colChr <= 'i' && rowChr >= '0' && rowChr <= '9') {
        int col = colChr - 'a';
        int row = rowChr - '0';

        //if (mf.rankFrom1) {
//            if (rowChr == '1' && str.length() > 2 && str[2] == '0') {
//                row = 10;
//            }
//            row--;
        //}

        return (9 - row) * 9 + col;
    }
    return -1;
}

const char* xqPieceTypeNames[2] = {
        ".kabrcnpx", // standard Bishop/kNight
        ".kaerchpx"  // set 2, Elephant, Horse
};

int xqCharactorToPieceType(char ch)
{
    if (ch >= 'A' && ch < 'Z') {
        ch += 'a' - 'A';
    }

    //  WARNING: hack charactor set
    // xqPieceTypeNames[0] = ".kabrcnpx";
    {
        if (ch == 'e') ch = 'b';
        else if (ch == 'h') ch = 'n';
    }

    auto pieceType = EMPTY;
    const char* p = strchr(xqPieceTypeNames[0], ch);
    int k = int(p - xqPieceTypeNames[0]);
    pieceType = k;
    return pieceType;
}

Move moveFromString_san(XqBoard& board, const std::string& str)
{
    std::string s;
    for(auto ch : str) {
        if (ch != '+' && ch != 'x' && ch != '*' && ch != '#') {
            if (ch < ' ' || ch == '.') ch = ' ';
            s += ch;
        }
    }

    auto from = -1, dest = -1, fromCol = -1, fromRow = -1;
    auto promotion = PieceType::empty;

//    auto p = s.find("=");
//    if (p != std::string::npos) {
//        if (s.length() == p + 1) {
//            return Move::illegalMove; // something wrong
//        }
//        char ch = s.at(p + 1);
//        promotion = board.xqCharactorToPieceType(ch);
//
//        s = s.substr(0, p);
//        if (s.size() < 2 || promotion == EMPTY) {
//            return Move::illegalMove;;
//        }
//    }

    int n = s.length(), t = 2;
    auto destString = s.substr(n - t, t);
//    if (mf.rankFrom1 && destString == "10" && n > 2) { // Rb10
//        t = 3;
//        destString = s.substr(n - t, t);
//    }
    dest = xqCoordinateStringToPos(destString);

    if (!board.isPositionValid(dest)) {
        return Move::illegalMove;;
    }

    if (n > t) {
        s = s.substr(0, n - t);
    } else {
        s = "";
    }

    auto pieceType = PieceType::pawn;

    if (!s.empty()) {
        size_t k = 0;
        char ch = s.at(0);
        if (ch >= 'A' && ch <= 'Z') {
            k++;
            pieceType = static_cast<PieceType>(xqCharactorToPieceType(ch));

            if (pieceType == PieceType::empty) {
                return Move::illegalMove;;
            }
        }

        auto left = s.length() - k;
        if (left > 0) {
            s = s.substr(k, left);
            if (left == 2) {
                from = xqCoordinateStringToPos(s);
            } else {
                char ch = s.at(0);
                if (isdigit(ch)) {
                    fromRow = 9 - ch + '0';

                    //if (mf.rankFrom1 && fromRow > 0) {
                    //    fromRow++; // careful: because of minus 9 -
                    //}
                } else if (ch >= 'a' && ch <= 'z') {
                    fromCol = ch - 'a';
                }
            }
        }
    }

//    assert((pieceType != JEIQI && promotion == EMPTY) || (pieceType == JEIQI && promotion > KING));

    if (from < 0) {
        auto moveList = board.gen(board.side);

        std::vector<Move> goodMoves;
        for (auto && m : moveList) {
            if (m.dest != dest || m.promotion != promotion ||
                board.getPiece(m.from).type != pieceType) {
                continue;
            }

            if ((fromRow < 0 && fromCol < 0) ||
                (fromRow >= 0 && board.getRow(m.from) == fromRow) ||
                (fromCol >= 0 && board.getColumn(m.from) == fromCol)) {
                goodMoves.push_back(m);
                from = m.from;
            }
        }

        if (goodMoves.size() > 1) {
            for(auto && m : goodMoves) {
                MoveFull move = board.createFullMove(m.from, dest, promotion);
                Hist hist;
                board.make(move, hist);
                auto incheck = board.isIncheck(board.side);
                board.takeBack(hist);
                if (!incheck) {
                    from = m.from;
                    break;
                }
            }
        }

        if (from < 0) {
            return Move::illegalMove;
        }
    }

//    assert(Move::isValidPromotion(promotion));
    return Move(from, dest, promotion);
}

// Check and make the move if it is legal
bool checkMake(XqBoard& board, int from, int dest, PieceType promotion)
{
    if (!MoveFull::isValid(from, dest)) {
        return false;
    }

    auto piece = board.getPiece(from);
    if (piece.isEmpty()
        || piece.side != board.side
        || piece.side == board.getPiece(dest).side
//        || !Move::isValidPromotion(promotion)
        ) {
        return false;
    }

    assert(board.isValid());

    auto moveList = board.gen(board.side);

    for (auto && move : moveList) {
        if (move.from != from || move.dest != dest) { /// || move.promotion != promotion) {
            continue;
        }

        auto theSide = board.side;
        auto fullmove = board.createFullMove(from, dest, promotion);
        board.make(fullmove); assert(board.side != theSide);

        if (board.isIncheck(theSide)) {
            board.takeBack();
            return false;
        }

//        createSanStringForLastMove(moveFormat);
//        createStringForLastMove(moveList, moveFormat);

        assert(board.isValid());
        return true;
    }

    return false;
}

bool fromMoveList(XqBoard& board, const std::string& moveText)
{
    auto parseMoveErrorCnt = 0;

    enum class State {
        none, move, comment, comment_variation, commentRestOfLine, evalsym, counter
    };

    auto st = State::none;

    std::vector<std::string> moveStringVec;
    std::map<size_t, std::string> commentMap;
    std::map<size_t, std::string> eSymMap;

    std::string moveString, comment, esym, variation_as_comment;

    auto ok = true;
    char ch = 0, prevch = 0;

    const char *p = moveText.c_str();
    for(size_t i = 0, len = strlen(p); i < len && ok; i++) {

        prevch = ch;
        ch = p[i];

//        auto q = p + i;
        switch (st) {
            case State::none:
            if (isalpha(ch)) { /// ch < 128 to avoid some Unicode
                    moveString = ch;
                    st = State::move;
                } else if (ch == '!' || ch == '?') {
                    esym = ch;
                    st = State::evalsym;
                } else if (ch == '{') {
                    // comments by semicolon or escape mechanism % in the header,
                    comment.clear();
                    st = State::comment;
                } else if (ch == ';' || (ch == '%' && (i == 0 || prevch == '\r' || prevch == '\n'))) {
                    // comments by semicolon or escape mechanism % in the header,
                    comment.clear();
                    st = State::commentRestOfLine;
                } else if (ch == '(') {
                        //variation_as_comment_level = 1;
                        comment.clear();
                        st = State::comment_variation;
                } else if (isdigit(ch)) {
                    st = State::counter;
                }
                break;

            case State::move:
                if (isalnum(ch) || ch == '=' || ch == '+' || (ch == '-' && (prevch == 'O' || prevch == '0'))
                        || ch == '.' || ch == '-'

                        ) { // O-O
                    moveString += ch;
                } else {
                    // 17. Qd4 gxf1=Q+
                    if (moveString.length() < 2 || moveString.length() > 8) {
                        ok = false;
                        if (len - i > 20) {
                            parseMoveErrorCnt++;
                        }
                        break;
                    }

                    moveStringVec.push_back(moveString);
                    moveString.clear();

                    i--;
                    st = State::none;
                }

                break;

            case State::evalsym:
                if (ch == '!' || ch == '?') {
                    esym += ch;
                    break;
                }
                eSymMap[moveStringVec.size()] = esym;
                esym.clear();

                i--;
                st = State::none;
                break;


            case State::comment_variation:
            case State::commentRestOfLine:
            case State::comment:
                if ((st == State::comment && ch == '}')
                    || (st == State::commentRestOfLine && (ch == '\n' || ch == '\r'))) {
//                    comment = rtrim(comment);
//
//                    if (flag & ParseMoveListFlag_smartComment) {
//                        smartFilterComment(comment);
//                    }
//                    if ((flag & ParseMoveListFlag_discardComment) == 0 && !comment.empty()) {
//
//                        auto k = moveStringVec.size();
//                        auto it = commentMap.find(k);
//                        if (it != commentMap.end()) {
//                            comment = it->second + " " + comment;
//                        }
//
//                        commentMap[k] = comment;
//                        comment.clear();
//                    }
                    st = State::none;
                    break;
                }

//                if ((flag & ParseMoveListFlag_discardComment) == 0) {
//                    if (!comment.empty() || ch < 0 || ch > ' ') { // trim left
//                        // remove new line charators in the middle
//                        if (ch == '\r' || ch == '\n') {
//                            ch = ' ';
//                            if (prevch == ' ') {
//                                break;
//                            }
//                        }
//                        comment += ch;
//                    }
//                }
                break;

            case State::counter:
                if (isalnum(ch)) {
                    break;
                }
                if (ch != '.') {// && ch != ')') {
                    i--;
                }

                st = State::none;
                break;

//            default:
//                break;
        }
    }

    if (moveString.size() > 1 && moveString.size() < 10) {
//        vr.groupIdx = groupIdxVec[vr.level];
        moveStringVec.push_back(moveString);
    }

    if (!esym.empty()) {
        eSymMap[moveStringVec.size()] = esym;
    }

    if (!comment.empty()) {
        commentMap[moveStringVec.size()] = comment;
    }

//    board.printOut("fromMoveList");
    for(size_t i = 0; i < moveStringVec.size(); i++) {
        auto ss = moveStringVec.at(i); assert(!ss.empty());
        
        /// Ignore string with $ such as $10 (from Lichess games)
        if (ss.at(0) == '$' && ss.length() > 1 && std::isdigit(ss.at(1))) {
            continue;
        }
        
        
        //        Move move = Move::illegalMove;
        auto move = moveFromString_san(board, ss);
        
        if (!checkMake(board, move.from, move.dest, move.promotion)) {
            parseMoveErrorCnt++;
            return false;
        }

    }
    
    return true;
}


bool parsePGN(const std::vector<std::string>& contentVec, std::map<std::string, std::string>& itemMap, std::vector<Hist>& moveVec, XqBoard& board)
{
    itemMap.clear();
    moveVec.clear();

    std::string moveText;
    for(auto && s : contentVec) {
        if (moveText.length() < 5) {
            auto p = s.find("[");
            if (p == 0) {
                p++;
                if (isalpha(s.at(p))) {
                    std::string key;
                    for(auto q = p + 1; q < s.length(); q++) {
                        if (s.at(q) <= ' ') {
                            key = s.substr(p, q - p);
                            break;
                        }
                    }
                    if (key.empty()) continue;
                    p = s.find("\"");
                    if (p == std::string::npos) continue;
                    p++;
                    auto q = s.find("\"", p);
                    if (q == std::string::npos) continue;
                    auto str = s.substr(p, q - p);
                    if (str.empty()) continue;

                    Funcs::toLower(key);
                    itemMap[key] = str;
                    continue;
                }
            }
        }
        moveText += " " + s;
    }

    std::string fen;
    if (itemMap.find("fen") != itemMap.end()) {
        fen = itemMap["fen"];
    }

    /// Fix the problem some Chinese PGN use Red/Black instead of White/Black
    if (itemMap.find("white") == itemMap.end()) {
        auto it = itemMap.find("red");
        if (it != itemMap.end()) {
            itemMap["white"] = it->second;
            itemMap.erase("red");
        }
    }

    board.newGame(fen);

    fromMoveList(board, moveText);
    std::string s = "last pos, histsz=" + std::to_string(board.histList.size());

    
    for(auto hist : board.histList) {
        moveVec.push_back(hist);
    }

    /// add white, black if missing
    if (!itemMap.empty()) {
        if (itemMap.find("white") == itemMap.end()) {
            itemMap["white"] = "White";
        }
        if (itemMap.find("black") == itemMap.end()) {
            itemMap["black"] = "Black";
        }
    }

    return true;
}

bool XqChaseJudge::testRules(const std::string& path)
{
    auto contentVec = readTextFileToArray(path);
    return testRules(contentVec);
}

bool XqChaseJudge::testRules(const std::vector<std::string>& _contentVec)
{
#ifndef _FELICITY_USE_HASH_
    std::cout << "Must defined _FELICITY_USE_HASH_" << std::endl;
    assert(false);
#endif
    std::vector<std::string> contentVec;

    /// remove all variations
    auto vCnt = 0;
    for(auto s : _contentVec) {
        for(size_t i = 0; i < s.length(); i++) {
            auto ch = s[i];
            if (ch == '(') {
                vCnt++;
            } else if (ch == ')') {
                vCnt--; assert(vCnt >= 0);
                s[i] = ' ';
            }

            if (vCnt) {
                s[i] = ' ';
            }
        }
        contentVec.push_back(s);
    }

    XqBoard board;

    std::map<std::string, std::string> itemMap;
    std::vector<Hist> moveVec;

    auto ok = parsePGN(contentVec, itemMap, moveVec, board);
    
    if (board.histList.size() < 4) {
        std::cout
                << "Event: " << itemMap["event"]
                << "\nRound: " << itemMap["round"]
                << std::endl;
        board.printOut("Incorrect XqChaseJudge::testRules");
        ok = parsePGN(contentVec, itemMap, moveVec, board);
    }
    assert(ok && board.quietCnt >= 5 && board.histList.size() >= 5);
//    board.printOut("after parsePGN");

    auto p = itemMap.find("result"); assert(p != itemMap.end());
    auto resultType = string2ResultType(p->second);

//    if (itemMap["event"] != "Asia rules 15*" || itemMap["round"] != "18") {
//        return true;
//    }
//    board.printOut("board");

    auto result = board.rule();
    auto correct = result.result == resultType;

//    if (correct) {
//        XqBoard board2(board);
//        assert(board2.isValid());
//        assert(board2.getHistListSize() == board.getHistListSize());
//
////        board.printOut("board before rotating");
//
//        board2.flip(FlipMode::rotate);
////        board2->printOut("board2 (rotated)");
//
//#ifdef _FELICITY_USE_HASH_
//        board2.setupHashKey();
//        assert(board2.isHashKeyValid());
//#endif
//
//        assert(board2.isValid());
//        assert(board2.getHistListSize() == board.getHistListSize());
//
////        board2->repetitionThreatHold = 1;
//
//        auto resultType2 = resultType == GameResultType::win ? GameResultType::loss : resultType == GameResultType::loss ? GameResultType::win : GameResultType::draw;
//        auto result2 = board2.rule();
//        correct = result2.result == resultType2;
//    }

    if (!correct) {
        std::cout
                << "Event: " << itemMap["event"]
                << "\nRound: " << itemMap["round"]
                << "\nruled result: " << result.toString() << ", pgn result: " << Result::resultType2String(resultType, true) << std::endl;
        board.printOut("Incorrect XqChaseJudge::testRules");
//        result = board.rule();
    }

    return correct;
}

void XqChaseJudge::testRules()
{

//    {
//        const std::string path = "/Users/nguyenpham/bsg/banksiagui/problems/working.pgn";
//        if (!testRules(path)) {
//            std::cout << "result incorrect!\n" << std::endl;
//        }
//
//        std::cout << "Test done" << std::endl;
//    }

    std::string axfFileName = "/Users/nguyenpham/bsg/banksiagui/doc/AsiaRule.pgn";
    auto vec = readTextFileToArray(axfFileName);

    std::vector<std::string> contentVec;
    std::map<std::string, std::string> itemMap;
    std::vector<Hist> moveVec;

    auto idx = 0, correctCnt = 0;

    auto ok = true;
    for(size_t i = 0, n = vec.size(); i < n; ++i) {
        auto line = vec.at(i);
        if (line.find("[Event") != std::string::npos || i + 1 == n) {
            if (!contentVec.empty()) {
                idx++;

                if (ok && testRules(contentVec)) {
                    correctCnt++;
                }

//                ok = false;
                contentVec.clear();
            }
        }

        if (!line.empty()) {
//            if (line.find("Round") != std::string::npos && line.find("21") != std::string::npos) {
//                ok = true;
//            }
            contentVec.push_back(line);
        }
    }

//    free(tmpBuf);
    std::cout << "DONE! total " << idx << ", correct " << correctCnt << std::endl;
    std::cout << "DONE!" << std::endl;
}


//} // namespace Chess

void XqChaseJudge::testRules2()
{
    XqBoard board;
    board.setFen("3aka3/1R7/9/9/9/9/1c7/9/4A4/2p1KA3 b 0 1");

//    board.setFenComplete();

    board.printOut();

    XqChaseJudge xqChaseJudge;
    
    auto side = Side::black, xside = Side::white;
    assert(xqChaseJudge.addBoard(board, side));

    board.make(board.createFullMove(55, 60));
    board.printOut();
    assert(xqChaseJudge.addBoard(board, xside));

    board.make(board.createFullMove(10, 15));
    board.printOut();
    assert(xqChaseJudge.addBoard(board, side));

    board.make(board.createFullMove(60, 55));
    board.printOut();
    assert(xqChaseJudge.addBoard(board, xside));

    
    board.make(board.createFullMove(15, 10));
    board.printOut();
    assert(xqChaseJudge.addBoard(board, side));

    auto r = xqChaseJudge.evaluate2();
    std::cout << "xqChaseJudge.evaluate: " << Result::resultType2String(r.result) << std::endl;

    auto result = xqChaseJudge.evaluate();

    auto result0 = board.rule();
    

    std::cout << "result: " << result.toString() << std::endl;

}

#endif // _FELICITY_XQ_
