%{
#include <cstdio>
#include <cstring>

// 词法分析头文件
#include "FlexLexer.h"

// bison生成的头文件
#include "BisonParser.h"

// 抽象语法树函数定义原型头文件
#include "AST.h"

#include "IntegerType.h"

static ast_node * clone_ast_node(const ast_node * node)
{
	if (!node) {
		return nullptr;
	}

	auto * copied = new ast_node(node->node_type, node->type, node->line_no);
	copied->integer_val = node->integer_val;
	copied->float_val = node->float_val;
	copied->name = node->name;
	copied->needScope = node->needScope;

	for (auto child: node->sons) {
		copied->insert_son_node(clone_ast_node(child));
	}

	return copied;
}

static ast_node * make_for_missing_marker()
{
	return ast_node::New(ast_operator_type::AST_OP_MAX);
}

static ast_node * make_inc_dec_expr(ast_node * lval, bool isInc, bool isPostfix)
{
	if (!lval) {
		return nullptr;
	}

	ast_node * rhsLVal = clone_ast_node(lval);
	digit_int_attr oneAttr{1, (uint32_t) lval->line_no};
	ast_node * oneNode = ast_node::New(oneAttr);
	auto op = isInc ? ast_operator_type::AST_OP_ADD : ast_operator_type::AST_OP_SUB;
	ast_node * calcNode = ast_node::New(op, rhsLVal, oneNode);
	ast_node * assignNode = ast_node::New(ast_operator_type::AST_OP_ASSIGN, lval, calcNode);

	if (!isPostfix) {
		return assignNode;
	}

	// 后缀表达式返回“旧值”：
	// x++ => (x = x + 1) - 1
	// x-- => (x = x - 1) + 1
	auto restoreOp = isInc ? ast_operator_type::AST_OP_SUB : ast_operator_type::AST_OP_ADD;
	return ast_node::New(restoreOp, assignNode, ast_node::New(oneAttr));
}

// LR分析失败时所调用函数的原型声明
void yyerror(char * msg);

// 指向创建的抽象语法树
static ast_node * ast_root;

// 向外部提供的接口函数，用于返回抽象语法树的根节点
ast_node * get_ast_root()
{
	return ast_root;
}

%}

// 联合体声明，用于后续终结符和非终结符号属性指定使用
%union {
    class ast_node * node;

    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
    struct type_attr type;
    int op_class;
};

// 文法的开始符号
%start  CompileUnit

// 指定文法的终结符号，<>可指定文法属性
// 对于单个字符的算符或者分隔符，在词法分析时可直返返回对应的ASCII码值，bison预留了255以内的值
// %token开始的符号称之为终结符，需要词法分析工具如flex识别后返回
// %type开始的符号称之为非终结符，需要通过文法产生式来定义
// %token或%type之后的<>括住的内容成为文法符号的属性，定义在前面的%union中的成员名字。
%token <integer_num> T_DIGIT
%token <var_id> T_ID
%token <type> T_INT
%token <type> T_VOID

// 关键或保留字 一词一类 不需要赋予语义属性
%token T_RETURN T_IF T_ELSE T_WHILE T_FOR T_BREAK T_CONTINUE
%token T_CONST T_STATIC

// 分隔符 一词一类 不需要赋予语义属性
%token T_SEMICOLON T_L_PAREN T_R_PAREN T_L_BRACE T_R_BRACE
%token T_L_BRACKET T_R_BRACKET
%token T_COMMA

// 运算符
%token T_ASSIGN T_SUB T_ADD T_MUL T_DIV T_MOD
%token T_EQ T_NE T_LT T_LE T_GT T_GE
%token T_AND T_OR T_NOT
%token T_INC T_DEC

// 解决悬挂else
%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

// 非终结符
// %type指定文法的非终结符号，<>可指定文法属性
%type <node> CompileUnit
%type <node> FuncDef
%type <node> FuncFParams FuncFParam
%type <node> FuncArrayDims FuncArrayDimsOpt
%type <node> Block
%type <node> BlockItemList
%type <node> BlockItem
%type <node> Statement
%type <node> ForInitOpt ForInit ForCondOpt ForStepOpt ForExpr
%type <node> Expr
%type <node> LVal
%type <node> ArraySubscripts
%type <node> VarDecl VarDeclExpr VarDef
%type <node> VarArrayDims
%type <node> LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp
%type <node> RealParamList
%type <type> BasicType
%type <op_class> AddOp
%type <op_class> MulOp
%%

// 编译单元可包含若干个函数与全局变量定义。要在语义分析时检查main函数存在
// compileUnit: (funcDef | varDecl)* EOF;
// bison不支持闭包运算，为便于追加修改成左递归方式
// compileUnit: funcDef | varDecl | compileUnit funcDef | compileUnit varDecl
CompileUnit : FuncDef {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = ast_node::New(ast_operator_type::AST_OP_COMPILE_UNIT, $1);

		// 设置到全局变量中
		ast_root = $$;
	}
	| VarDecl {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = ast_node::New(ast_operator_type::AST_OP_COMPILE_UNIT, $1);
		ast_root = $$;
	}
	| CompileUnit FuncDef {

		// 把函数定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	| CompileUnit VarDecl {
		// 把变量定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	;

// 函数定义，目前支持整数返回类型，支持形参
FuncDef : BasicType T_ID T_L_PAREN T_R_PAREN Block  {

		// 函数返回类型
		type_attr funcReturnType = $1;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$5
		ast_node * blockNode = $5;

		// 形参结点没有，设置为空指针
		ast_node * formalParamsNode = nullptr;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参(实际上无)
		// create_func_def函数内会释放funcId中指向的标识符空间，切记，之后不要再释放，之前一定要是通过strdup函数或者malloc分配的空间
		$$ = ast_node::create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	| BasicType T_ID T_L_PAREN FuncFParams T_R_PAREN Block {
		// 函数定义带形参
		type_attr funcReturnType = $1;
		var_id_attr funcId = $2;
		ast_node * formalParamsNode = $4;
		ast_node * blockNode = $6;
		$$ = ast_node::create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	;

// 函数形参列表：FuncFParams : FuncFParam | FuncFParams T_COMMA FuncFParam
FuncFParams : FuncFParam {
		$$ = ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS, $1);
	}
	| FuncFParams T_COMMA FuncFParam {
		$$ = $1->insert_son_node($3);
	}
	;

// 单个函数形参：BasicType T_ID
FuncFParam : BasicType T_ID {
		if ($1.type == BasicType::TYPE_VOID) {
			yyerror("function parameter type cannot be void");
			YYERROR;
		}
		ast_node * type_node = ast_node::create_type_node($1);
		ast_node * name_node = ast_node::New(std::string($2.id), $2.lineno);
		free($2.id);
		$$ = ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, name_node);
		$$->type = type_node->type;
		$$->name = name_node->name;
	}
	| BasicType T_ID T_L_BRACKET T_R_BRACKET FuncArrayDimsOpt {
		if ($1.type == BasicType::TYPE_VOID) {
			yyerror("function parameter type cannot be void");
			YYERROR;
		}
		ast_node * name_node = ast_node::New(std::string($2.id), $2.lineno);
		free($2.id);

		ast_node * dims_node = ast_node::createArrayDimsNode(ast_node::New(digit_int_attr{0, $2.lineno}));
		if ($5) {
			for (auto child: $5->sons) {
				dims_node->insert_son_node(child);
			}
		}

		ast_node * type_node = ast_node::create_type_node($1, dims_node, true);
		$$ = ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, name_node);
		$$->type = type_node->type;
		$$->name = name_node->name;
	}
	;

FuncArrayDimsOpt :
		{
			$$ = nullptr;
		}
	| FuncArrayDims
		{
			$$ = $1;
		}
	;

FuncArrayDims : T_L_BRACKET T_DIGIT T_R_BRACKET {
		$$ = ast_node::New(ast_operator_type::AST_OP_ARRAY_DIMS, ast_node::New($2));
	}
	| FuncArrayDims T_L_BRACKET T_DIGIT T_R_BRACKET {
		$$ = $1->insert_son_node(ast_node::New($3));
	}
	;

// 语句块的文法Block ： T_L_BRACE BlockItemList? T_R_BRACE
// 其中?代表可有可无，在bison中不支持，需要拆分成两个产生式
// Block ： T_L_BRACE T_R_BRACE | T_L_BRACE BlockItemList T_R_BRACE
Block : T_L_BRACE T_R_BRACE {
		// 语句块没有语句

		// 为了方便创建一个空的Block节点
		$$ = ast_node::New(ast_operator_type::AST_OP_BLOCK);
	}
	| T_L_BRACE BlockItemList T_R_BRACE {
		// 语句块含有语句

		// BlockItemList归约时内部创建Block节点，并把语句加入，这里不创建Block节点
		$$ = $2;
	}
	;

// 语句块内语句列表的文法：BlockItemList : BlockItem+
// Bison不支持正闭包，需修改成左递归形式，便于属性的传递与孩子节点的追加
// 左递归形式的文法为：BlockItemList : BlockItem | BlockItemList BlockItem
BlockItemList : BlockItem {
		// 第一个左侧的孩子节点归约成Block节点，后续语句可持续作为孩子追加到Block节点中
		// 创建一个AST_OP_BLOCK类型的中间节点，孩子为Statement($1)
		$$ = ast_node::New(ast_operator_type::AST_OP_BLOCK, $1);
	}
	| BlockItemList BlockItem {
		// 把BlockItem归约的节点加入到BlockItemList的节点中
		$$ = $1->insert_son_node($2);
	}
	;


// 语句块中子项的文法：BlockItem : Statement
// 目前只支持语句,后续可增加支持变量定义
BlockItem : Statement  {
		// 语句节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	| VarDecl {
		// 变量声明节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	;

// 变量声明语句
// 语法：varDecl: basicType varDef (T_COMMA varDef)* T_SEMICOLON
// 因Bison不支持闭包运算符，因此需要修改成左递归，修改后的文法为：
// VarDecl : VarDeclExpr T_SEMICOLON
// VarDeclExpr: BasicType VarDef | VarDeclExpr T_COMMA varDef
VarDecl : VarDeclExpr T_SEMICOLON {
		$$ = $1;
	}
	;

// 变量声明表达式，可支持逗号分隔定义多个
VarDeclExpr: BasicType VarDef {
		if ($1.type == BasicType::TYPE_VOID) {
			yyerror("variable type cannot be void");
			YYERROR;
		}

		ast_node * dims_node = nullptr;
		if (!$2->sons.empty() && $2->sons[0] && $2->sons[0]->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			dims_node = $2->sons[0];
		}

		// 创建类型节点
		ast_node * type_node = ast_node::create_type_node($1, dims_node);

		// 创建变量定义节点
		ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, $2);
		decl_node->type = type_node->type;
		decl_node->isConstDecl = false;
		decl_node->isStaticDecl = false;

		// 创建变量声明语句，并加入第一个变量
		$$ = ast_node::create_var_decl_stmt_node(decl_node);
	}
	| T_CONST BasicType VarDef {
		if ($2.type == BasicType::TYPE_VOID) {
			yyerror("variable type cannot be void");
			YYERROR;
		}

		ast_node * dims_node = nullptr;
		if (!$3->sons.empty() && $3->sons[0] && $3->sons[0]->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			dims_node = $3->sons[0];
		}

		// 创建类型节点
		ast_node * type_node = ast_node::create_type_node($2, dims_node);

		// 创建变量定义节点
		ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		decl_node->type = type_node->type;
		decl_node->isConstDecl = true;
		decl_node->isStaticDecl = false;

		// 创建变量声明语句，并加入第一个变量
		$$ = ast_node::create_var_decl_stmt_node(decl_node);
	}
	| T_STATIC BasicType VarDef {
		if ($2.type == BasicType::TYPE_VOID) {
			yyerror("variable type cannot be void");
			YYERROR;
		}

		ast_node * dims_node = nullptr;
		if (!$3->sons.empty() && $3->sons[0] && $3->sons[0]->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			dims_node = $3->sons[0];
		}

		ast_node * type_node = ast_node::create_type_node($2, dims_node);
		ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		decl_node->type = type_node->type;
		decl_node->isConstDecl = false;
		decl_node->isStaticDecl = true;

		$$ = ast_node::create_var_decl_stmt_node(decl_node);
	}
	| T_STATIC T_CONST BasicType VarDef {
		if ($3.type == BasicType::TYPE_VOID) {
			yyerror("variable type cannot be void");
			YYERROR;
		}

		ast_node * dims_node = nullptr;
		if (!$4->sons.empty() && $4->sons[0] && $4->sons[0]->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			dims_node = $4->sons[0];
		}

		ast_node * type_node = ast_node::create_type_node($3, dims_node);
		ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, $4);
		decl_node->type = type_node->type;
		decl_node->isConstDecl = true;
		decl_node->isStaticDecl = true;

		// 创建变量声明语句，并加入第一个变量
		$$ = ast_node::create_var_decl_stmt_node(decl_node);
	}
	| VarDeclExpr T_COMMA VarDef {
		// 创建类型节点：每个VarDef独立按是否带数组维度确定类型
		type_attr baseTypeAttr;
		baseTypeAttr.lineno = $3->line_no;
		if ($1->type && $1->type->isVoidType()) {
			baseTypeAttr.type = BasicType::TYPE_VOID;
		} else {
			baseTypeAttr.type = BasicType::TYPE_INT;
		}

		ast_node * dims_node = nullptr;
		if (!$3->sons.empty() && $3->sons[0] && $3->sons[0]->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			dims_node = $3->sons[0];
		}

		ast_node * type_node = ast_node::create_type_node(baseTypeAttr, dims_node);

		// 创建变量定义节点
		ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		decl_node->type = type_node->type;
		if (!$1->sons.empty()) {
			decl_node->isConstDecl = $1->sons[0]->isConstDecl;
			decl_node->isStaticDecl = $1->sons[0]->isStaticDecl;
		}

		// 插入到变量声明语句
		$$ = $1->insert_son_node(decl_node);
	}
	;

// 变量定义包含变量名，实际上还有初值，这里没有实现。
VarDef : T_ID {
		// 变量ID

		$$ = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
	}
	| T_ID T_ASSIGN Expr {
		$$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
		$$->insert_son_node($3);
		free($1.id);
	}
	| T_ID VarArrayDims {
		$$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
		$$->insert_son_node($2);
		free($1.id);
	}
	| T_ID VarArrayDims T_ASSIGN Expr {
		$$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
		$$->insert_son_node($2);
		$$->insert_son_node($4);
		free($1.id);
	}
	;

VarArrayDims : T_L_BRACKET T_DIGIT T_R_BRACKET {
		$$ = ast_node::New(ast_operator_type::AST_OP_ARRAY_DIMS, ast_node::New($2));
	}
	| VarArrayDims T_L_BRACKET T_DIGIT T_R_BRACKET {
		$$ = $1->insert_son_node(ast_node::New($3));
	}
	;

// 基本类型，目前只支持整型
BasicType: T_INT {
		$$ = $1;
	}
	| T_VOID {
		$$ = $1;
	}
	;

// 语句文法：statement:T_RETURN expr T_SEMICOLON | lVal T_ASSIGN expr T_SEMICOLON
// | block | expr? T_SEMICOLON
// 支持返回语句、赋值语句、语句块、表达式语句
// 其中表达式语句可支持空语句，由于bison不支持?，修改成两条
Statement : T_RETURN Expr T_SEMICOLON {
		// 返回语句

		// 创建返回节点AST_OP_RETURN，其孩子为Expr，即$2
		$$ = ast_node::New(ast_operator_type::AST_OP_RETURN, $2);
	}
	| T_RETURN T_SEMICOLON {
		// 无返回值的return语句
		$$ = ast_node::New(ast_operator_type::AST_OP_RETURN);
	}
	| LVal T_ASSIGN Expr T_SEMICOLON {
		// 赋值语句

		// 创建一个AST_OP_ASSIGN类型的中间节点，孩子为LVal($1)和Expr($3)
		$$ = ast_node::New(ast_operator_type::AST_OP_ASSIGN, $1, $3);
	}
	| Block {
		// 语句块

		// 内部已创建block节点，直接传递给Statement
		$$ = $1;
	}
	| Expr T_SEMICOLON {
		// 表达式语句

		// 内部已创建表达式，直接传递给Statement
		$$ = $1;
	}
	| T_SEMICOLON {
		// 空语句

		// 直接返回空指针，需要再把语句加入到语句块时要注意判断，空语句不要加入
		$$ = nullptr;
	}
	| T_IF T_L_PAREN Expr T_R_PAREN Statement %prec LOWER_THAN_ELSE {
		// if语句
		$$ = ast_node::New(ast_operator_type::AST_OP_IF, $3, $5);
	}
	| T_IF T_L_PAREN Expr T_R_PAREN Statement T_ELSE Statement {
		// if-else语句
		$$ = ast_node::New(ast_operator_type::AST_OP_IF, $3, $5, $7);
	}
	| T_WHILE T_L_PAREN Expr T_R_PAREN Statement {
		// while语句
		$$ = ast_node::New(ast_operator_type::AST_OP_WHILE, $3, $5);
	}
	| T_FOR T_L_PAREN ForInitOpt T_SEMICOLON ForCondOpt T_SEMICOLON ForStepOpt T_R_PAREN Statement {
		ast_node * initNode = $3 ? $3 : make_for_missing_marker();
		ast_node * condNode = $5 ? $5 : make_for_missing_marker();
		ast_node * stepNode = $7 ? $7 : make_for_missing_marker();
		$$ = ast_node::New(ast_operator_type::AST_OP_FOR, initNode, condNode, stepNode, $9);
	}
	| T_BREAK T_SEMICOLON {
		// break语句
		$$ = ast_node::New(ast_operator_type::AST_OP_BREAK);
	}
	| T_CONTINUE T_SEMICOLON {
		// continue语句
		$$ = ast_node::New(ast_operator_type::AST_OP_CONTINUE);
	}
	;

// 表达式文法 expr : lOrExp
Expr : LOrExp {
		$$ = $1;
	}
	;

// 逻辑或表达式
LOrExp : LAndExp {
		$$ = $1;
	}
	| LOrExp T_OR LAndExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_LOR, $1, $3);
	}
	;

// 逻辑与表达式
LAndExp : EqExp {
		$$ = $1;
	}
	| LAndExp T_AND EqExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_LAND, $1, $3);
	}
	;

// 相等性表达式
EqExp : RelExp {
		$$ = $1;
	}
	| EqExp T_EQ RelExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_EQ, $1, $3);
	}
	| EqExp T_NE RelExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_NE, $1, $3);
	}
	;

// 关系表达式
RelExp : AddExp {
		$$ = $1;
	}
	| RelExp T_LT AddExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_LT, $1, $3);
	}
	| RelExp T_LE AddExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_LE, $1, $3);
	}
	| RelExp T_GT AddExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_GT, $1, $3);
	}
	| RelExp T_GE AddExp {
		$$ = ast_node::New(ast_operator_type::AST_OP_GE, $1, $3);
	}
	;

// 加减表达式文法：addExp: mulExp (addOp mulExp)*
AddExp : MulExp {
		// 直接传递给归约后的节点
		$$ = $1;
	}
	| AddExp AddOp MulExp {
		// 左递归形式可通过加减连接多个乘法表达式

		// 创建加减运算节点，孩子为AddExp($1)和MulExp($3)
		$$ = ast_node::New(ast_operator_type($2), $1, $3);
	}
	;

// 加减运算符
AddOp: T_ADD {
		$$ = (int)ast_operator_type::AST_OP_ADD;
	}
	| T_SUB {
		$$ = (int)ast_operator_type::AST_OP_SUB;
	}
	;

// 乘除模表达式文法：mulExp: unaryExp | mulExp mulOp unaryExp
MulExp : UnaryExp {
		// 一元表达式
		$$ = $1;
	}
	| MulExp MulOp UnaryExp {
		// 左递归形式可通过乘除模连接多个一元表达式
		$$ = ast_node::New(ast_operator_type($2), $1, $3);
	}
	;

// 乘除模运算符
MulOp: T_MUL {
		$$ = (int)ast_operator_type::AST_OP_MUL;
	}
	| T_DIV {
		$$ = (int)ast_operator_type::AST_OP_DIV;
	}
	| T_MOD {
		$$ = (int)ast_operator_type::AST_OP_MOD;
	}
	;

// 目前一元表达式可以为基本表达式、函数调用，其中函数调用的实参可有可无
// 其文法为：unaryExp: primaryExp | T_SUB unaryExp | T_ID T_L_PAREN realParamList? T_R_PAREN
// 由于bison不支持？表达，因此变更后的文法为：
// unaryExp: primaryExp | T_SUB unaryExp | T_ID T_L_PAREN T_R_PAREN | T_ID T_L_PAREN realParamList T_R_PAREN
UnaryExp : PrimaryExp {
		// 基本表达式

		// 传递到归约后的UnaryExp上
		$$ = $1;
	}
	| T_SUB UnaryExp {
		// 单目求负，负数字面量按“无符号数 + 求负符号”处理
		digit_int_attr zeroAttr{0, $2 ? $2->line_no : -1};
		ast_node * zeroNode = ast_node::New(zeroAttr);
		$$ = ast_node::New(ast_operator_type::AST_OP_SUB, zeroNode, $2);
	}
	| T_NOT UnaryExp {
		// 逻辑非
		$$ = ast_node::New(ast_operator_type::AST_OP_LNOT, $2);
	}
	| T_INC LVal {
		$$ = make_inc_dec_expr($2, true, false);
	}
	| T_DEC LVal {
		$$ = make_inc_dec_expr($2, false, false);
	}
	| LVal T_INC {
		$$ = make_inc_dec_expr($1, true, true);
	}
	| LVal T_DEC {
		$$ = make_inc_dec_expr($1, false, true);
	}
	| T_ID T_L_PAREN T_R_PAREN {
		// 没有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = nullptr;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参为空，但函数内部会创建实参列表节点，无孩子
		$$ = ast_node::create_func_call(name_node, paramListNode);

	}
	| T_ID T_L_PAREN RealParamList T_R_PAREN {
		// 含有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = $3;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参不为空
		$$ = ast_node::create_func_call(name_node, paramListNode);
	}
	;

ForInitOpt :
		{
			$$ = nullptr;
		}
	| ForInit {
			$$ = $1;
		}
	;

ForInit : Expr {
		$$ = $1;
	}
	| ForExpr {
		$$ = $1;
	}
	| VarDeclExpr {
		$$ = $1;
	}
	;

ForCondOpt :
		{
			$$ = nullptr;
		}
	| Expr {
			$$ = $1;
		}
	;

ForStepOpt :
		{
			$$ = nullptr;
		}
	| Expr {
			$$ = $1;
		}
	| ForExpr {
			$$ = $1;
		}
	;

ForExpr : LVal T_ASSIGN Expr {
		$$ = ast_node::New(ast_operator_type::AST_OP_ASSIGN, $1, $3);
	}
	;

// 基本表达式支持无符号整型字面量、带括号的表达式、具有左值属性的表达式
// 其文法为：primaryExp: T_L_PAREN expr T_R_PAREN | T_DIGIT | lVal
PrimaryExp :  T_L_PAREN Expr T_R_PAREN {
		// 带有括号的表达式
		$$ = $2;
	}
	| T_DIGIT {
        	// 无符号整型字面量

		// 创建一个无符号整型的终结符节点
		$$ = ast_node::New($1);
	}
	| LVal  {
		// 具有左值的表达式

		// 直接传递到归约后的非终结符号PrimaryExp
		$$ = $1;
	}
	;

// 实参表达式支持逗号分隔的若干个表达式
// 其文法为：realParamList: expr (T_COMMA expr)*
// 由于Bison不支持闭包运算符表达，修改成左递归形式的文法
// 左递归文法为：RealParamList : Expr | 左递归文法为：RealParamList T_COMMA expr
RealParamList : Expr {
		// 创建实参列表节点，并把当前的Expr节点加入
		$$ = ast_node::New(ast_operator_type::AST_OP_FUNC_REAL_PARAMS, $1);
	}
	| RealParamList T_COMMA Expr {
		// 左递归增加实参表达式
		$$ = $1->insert_son_node($3);
	}
	;

// 左值表达式，目前只支持变量名，实际上还有下标变量
LVal : T_ID {
		// 变量名终结符

		// 创建变量名终结符节点
		$$ = ast_node::New($1);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
	}
	| T_ID ArraySubscripts {
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});
		$$ = ast_node::createArrayAccessNode(id_node, $2);
		free($1.id);
	}
	;

// 数组下标访问，支持下标为一般表达式
ArraySubscripts : T_L_BRACKET Expr T_R_BRACKET {
		$$ = ast_node::New(ast_operator_type::AST_OP_ARRAY_DIMS, $2);
	}
	| ArraySubscripts T_L_BRACKET Expr T_R_BRACKET {
		$$ = $1->insert_son_node($3);
	}
	;

%%

// 语法识别错误要调用函数的定义
void yyerror(char * msg)
{
    printf("Line %d: %s\n", yylineno, msg);
}
