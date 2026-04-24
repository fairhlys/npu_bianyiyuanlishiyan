///
/// @file RecursiveDescentFlex.cpp
/// @brief 词法分析的手动实现源文件
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#include "RecursiveDescentParser.h"
#include "Common.h"

/// @brief 词法分析的行号信息
int64_t rd_line_no = 1;

/// @brief 词法分析的token对应的字符识别
std::string tokenValue;

/// @brief 输入源文件指针
FILE * rd_filein;

/// @brief 关键字与Token类别的数据结构
struct KeywordToken {
	std::string name;
	enum RDTokenType type;
};

/// @brief  关键字与Token对应表
static KeywordToken allKeywords[] = {
	{"int", RDTokenType::T_INT},
	{"return", RDTokenType::T_RETURN},
};

/// @brief 在标识符中检查是否时关键字，若是关键字则返回对应关键字的Token，否则返回T_ID
/// @param id 标识符
/// @return Token
static RDTokenType getKeywordToken(std::string id)
{
	//如果在allkeywords中找到，则说明为关键字
	for (auto & keyword: allKeywords) {
		if (keyword.name == id) {
			return keyword.type;
		}
	}
	// 如果不再allkeywords中，说明是标识符
	return RDTokenType::T_ID;
}

/// @brief 词法文法，获取下一个Token
/// @return  Token，值保存在rd_lval中
int rd_flex()
{
	int c;				// 扫描的字符
	int tokenKind = -1; // Token的值

	// 忽略空白符号，主要有空格，TAB键和换行符
	while ((c = fgetc(rd_filein)) == ' ' || c == '\t' || c == '\n' || c == '\r') {

		// 支持Linux/Windows/Mac系统的行号分析
		// Windows：\r\n
		// Mac: \n
		// Unix(Linux): \r
		if (c == '\r') {
			c = fgetc(rd_filein);
			rd_line_no++;
			if (c != '\n') {
				// 不是\n，则回退
				if (c != EOF) {
					ungetc(c, rd_filein);
				}
			}
		} else if (c == '\n') {
			rd_line_no++;
		}
	}

	// 文件结束符
	if (c == EOF) {
		// 返回文件结束符
		return RDTokenType::T_EOF;
	}

	// TODO 请自行实现删除源文件中的注释，含单行注释和多行注释等
	if (c == '/') {
		int next = fgetc(rd_filein);
		if (next == '/') {
			while ((c = fgetc(rd_filein)) != EOF && c != '\n' && c != '\r') {
			}
			if (c == '\r') {
				int maybeNl = fgetc(rd_filein);
				if (maybeNl != '\n' && maybeNl != EOF) {
					ungetc(maybeNl, rd_filein);
				}
			}
			if (c == '\n' || c == '\r') {
				rd_line_no++;
			}
			return rd_flex();
		} else if (next == '*') {
			int prev = 0;
			while ((c = fgetc(rd_filein)) != EOF) {
				if (c == '\n') {
					rd_line_no++;
				}
				if (prev == '*' && c == '/') {
					return rd_flex();
				}
				prev = c;
			}
			return RDTokenType::T_EOF;
		} else {
			if (next != EOF) {
				ungetc(next, rd_filein);
			}
		}
	}

	// 处理数字
	if (isdigit(c)) {

		rd_lval.integer_num.lineno = rd_line_no;

		if (c == '0') {
			int next = fgetc(rd_filein);

			if ((next == 'x') || (next == 'X')) {

				// 16进制：0x/0X 开头，至少一个16进制数字
				std::string digits;
				while (isxdigit(c = fgetc(rd_filein))) {
					digits.push_back((char) c);
				}

				if (digits.empty()) {
					printf("Line(%lld): Invalid hex literal 0%c\n", (long long) rd_line_no, (char) next);
					tokenKind = RDTokenType::T_ERR;
				} else {
					tokenValue = "0";
					tokenValue.push_back((char) next);
					tokenValue += digits;
					rd_lval.integer_num.val = (uint32_t) strtoul(tokenValue.c_str(), nullptr, 16);
					tokenKind = RDTokenType::T_DIGIT;
				}

				if (c != EOF) {
					ungetc(c, rd_filein);
				}
			} else if ((next != EOF) && isdigit(next)) {

				// 8进制：0开头
				std::string digits;
				digits.push_back((char) next);
				bool validOct = (next >= '0' && next <= '7');

				while (isdigit(c = fgetc(rd_filein))) {
					digits.push_back((char) c);
					if (c < '0' || c > '7') {
						validOct = false;
					}
				}

				tokenValue = "0" + digits;

				if (!validOct) {
					printf("Line(%lld): Invalid octal literal %s\n", (long long) rd_line_no, tokenValue.c_str());
					tokenKind = RDTokenType::T_ERR;
				} else {
					rd_lval.integer_num.val = (uint32_t) strtoul(tokenValue.c_str(), nullptr, 8);
					tokenKind = RDTokenType::T_DIGIT;
				}

				if (c != EOF) {
					ungetc(c, rd_filein);
				}
			} else {

				// 单个0
				rd_lval.integer_num.val = 0;
				tokenValue = "0";
				tokenKind = RDTokenType::T_DIGIT;
				if (next != EOF) {
					ungetc(next, rd_filein);
				}
			}
		} else {

			// 10进制无符号整数
			rd_lval.integer_num.val = c - '0';
			while (((c = fgetc(rd_filein)) != EOF) && isdigit(c)) {
				rd_lval.integer_num.val = rd_lval.integer_num.val * 10 + c - '0';
			}

			tokenValue = std::to_string(rd_lval.integer_num.val);
			tokenKind = RDTokenType::T_DIGIT;
			if (c != EOF) {
				if (c != EOF) {
					ungetc(c, rd_filein);
				}
			}
		}
	} else if (c == '(') {
		// 识别字符(
		tokenKind = RDTokenType::T_L_PAREN;
		// 存储字符(
		tokenValue = "(";
	} else if (c == ')') {
		// 识别字符)
		tokenKind = RDTokenType::T_R_PAREN;
		// 存储字符)
		tokenValue = ")";
	} else if (c == '{') {
		// 识别字符{
		tokenKind = RDTokenType::T_L_BRACE;
		// 存储字符{
		tokenValue = "{";
	} else if (c == '}') {
		// 识别字符}
		tokenKind = RDTokenType::T_R_BRACE;
		// 存储字符}
		tokenValue = "}";
	} else if (c == '[') {
		// 识别字符[
		tokenKind = RDTokenType::T_L_BRACKET;
		// 存储字符[
		tokenValue = "[";
	} else if (c == ']') {
		// 识别字符]
		tokenKind = RDTokenType::T_R_BRACKET;
		// 存储字符]
		tokenValue = "]";
	} else if (c == ';') {
		// 识别字符;
		tokenKind = RDTokenType::T_SEMICOLON;
		// 存储字符;
		tokenValue = ";";
	} else if (c == '+') {
		// 识别字符+
		tokenKind = RDTokenType::T_ADD;
		// 存储字符+
		tokenValue = "+";
	} else if (c == '-') {
		// 识别字符-
		tokenKind = RDTokenType::T_SUB;
		// 存储字符-
		tokenValue = "-";
	} else if (c == '*') {
		// 识别字符*
		tokenKind = RDTokenType::T_MUL;
		tokenValue = "*";
	} else if (c == '/') {
		// 识别字符/
		tokenKind = RDTokenType::T_DIV;
		tokenValue = "/";
	} else if (c == '%') {
		// 识别字符%
		tokenKind = RDTokenType::T_MOD;
		tokenValue = "%";
	} else if (c == '=') {
		// 识别字符=
		tokenKind = RDTokenType::T_ASSIGN;
	} else if (c == ',') {
		// 识别字符;
		tokenKind = RDTokenType::T_COMMA;
		// 存储字符,
		tokenValue = ",";
	} else if (isLetterUnderLine(c)) {
		// 识别标识符，包含关键字/保留字或自定义标识符

		// 最长匹配标识符
		std::string name;

		do {
			// 记录字符
			name.push_back(c);
			c = fgetc(rd_filein);
		} while (isLetterDigitalUnderLine(c));

		// 存储标识符
		tokenValue = name;

		// 多读的字符恢复，下次可继续读到该字符
		ungetc(c, rd_filein);

		// 检查是否是关键字，若是则返回对应的Token，否则返回T_ID
		tokenKind = getKeywordToken(name);
		if (tokenKind == RDTokenType::T_ID) {
			// 自定义标识符

			// 设置ID的值
			rd_lval.var_id.id = strdup(name.c_str());

			// 设置行号
			rd_lval.var_id.lineno = rd_line_no;
		} else if (tokenKind == RDTokenType::T_INT) {
			// int关键字

			// 设置类型与行号
			rd_lval.type.type = BasicType::TYPE_INT;
			rd_lval.type.lineno = rd_line_no;
		}
	} else {
		printf("Line(%lld): Invalid char %s\n", (long long) rd_line_no, tokenValue.c_str());
		tokenKind = RDTokenType::T_ERR;
	}

	// Token的类别
	return tokenKind;
}
