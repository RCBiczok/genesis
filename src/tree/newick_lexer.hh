#ifndef GNS_TREE_NEWICKLEXER_H_
#define GNS_TREE_NEWICKLEXER_H_

/**
 * @brief
 *
 * @file
 * @ingroup tree
 */

#include "node.hh"
#include "../utils/lexer.hh"

namespace genesis {
namespace utils {

// TODO
// Graphic characters: ! " # % & ' ( ) * + , - . / : ; < = > ? [ \ ] ^ _ { | } ~

class NewickLexer : public Lexer
{
public:
    NewickLexer() {
        SetCharType(kComment,  "[]");
        SetCharType(kNumber,   ".");
        SetCharType(kString,   "'");
        SetCharType(kOperator, ",:;");
        SetCharType(kBracket,  "()");
        SetCharType(kTag,      "{}");

        include_whitespace        = false;
        include_comments          = false;
        glue_sign_to_number       = true;
        trim_quotation_marks      = true;
        use_string_escape         = false;
        use_string_doubled_quotes = true;
    }

    //~ bool Analyze(const std::string& text)
    //~ {
        //~ return Lexer::Analyze(text);
    //~ }

protected:
    inline bool ScanComment()
    {
        size_t start = GetPosition();
        bool   found = ScanFromTo("[", "]");
        if (found && include_comments) {
            PushToken(kComment, start, GetPosition());
        }
        return found;
    }
};



} // namespace utils
} // namespace genesis

#endif // include guard
