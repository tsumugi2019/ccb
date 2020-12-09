#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum{
    TK_RESERVED,    //記号
    TK_NUM,         //整数
    TK_EOF,         //入力の終わり
} TokenKind;

//抽象構文木のノードの種類
typedef enum{
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_NUM,     //数
} NodeKind;

typedef struct Token Token;

//トークンの構造体
struct Token{
    TokenKind kind; //トークンの種類
    Token* next;    //次のトークン
    int val;        //整数トークンの場合、その数値
    char* str;      //トークン文字列    
};

typedef struct Node Node;

//抽象構文木のノードの構造体
struct Node{
    NodeKind kind;  //ノードの種類
    Node* lhs;      //左辺
    Node* rhs;      //右辺
    int val;        //整数ノードの場合、その数値
};

//現在着目しているトークン
Token* token;

//入力プログラム
char* user_input;

//エラーの箇所を報告する関数
void error_at(char* loc, char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");   //pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//エラーを報告する関数
void error(char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときは、トークンを1つ読み進めて
//trueを返す。そうでなければfalseを返す。
bool consume(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return token;
}

//次のトークンが期待してる記号のときは、トークンを1つ読み進める。
//そうでなければエラーを報告する。
void expect(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op)
        error_at(token->str, "'%c'ではありません", op);
    token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
//そうでなければエラーを報告する。
int expect_number(){
    if(token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

//入力が終了したか
bool at_eof(){
    return token->kind == TK_EOF;
}

//新しいトークンを作成してcurに繋げる
Token* new_token(TokenKind kind, Token* cur, char* str){
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズしてそれを返す
Token* tokenize(char* p){
    Token head;
    head.next = NULL;
    Token* cur = &head;

    while(*p){
        //空白文字はスキップ
        if(isspace(*p)){
            p++;
            continue;
        }
        //記号
        if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')'){
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }
        //数
        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        //どれでもない
        error_at(p, "トークナイズできません");
    }
    //最後にEOFを追加
    new_token(TK_EOF, cur, p);
    return head.next;
}

//新しい演算子ノードを追加してそれを返す
Node* new_node(NodeKind kind, Node* lhs, Node* rhs){
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

//新しい整数ノードを追加してそれを返す
Node* new_node_num(int val){
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

//パーサ関数のプロトタイプ宣言
Node* expr();
Node* mul();
Node* primary();

//exprのパーサ
Node* expr(){
    Node* node = mul();
    for(;;){
        if(consume('+'))
            node = new_node(ND_ADD, node, mul());
        else if(consume('-'))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

//mulのパーサ
Node* mul(){
    Node* node = primary();
    for(;;){
        if(consume('*'))
            node = new_node(ND_MUL, node, primary());
        else if(consume('/'))
            node = new_node(ND_DIV, node, primary());
        else
            return node;
    }
}

//primaryのパーサ
Node* primary(){
    //次のトークンが'('なら、'(' expr ')'のはず
    if(consume('(')){
        Node* node = expr();
        expect(')');
        return node;
    }

    //そうでなければ数値のはず
    return new_node_num(expect_number());
}

//抽象構文木からアセンブリコードを生成する関数
void gen(Node* node){
    //整数なら側プッシュしてリターン
    if(node->kind == ND_NUM){
        printf("    push %d\n", node->val);
        return;
    }

    //左部分木と右部分木のコードを生成
    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch(node->kind){
    case ND_ADD: printf("   add rax, rdi\n"); break;
    case ND_SUB: printf("   sub rax, rdi\n"); break;
    case ND_MUL: printf("   imul rax, rdi\n"); break;
    case ND_DIV: printf("   cqo\n"); printf("   idiv rdi\n"); break;
    }
    printf("    push rax\n");
}

int main(int argc, char** argv){

    if(argc != 2){
        error("引数の個数が正しくありません\n");
        return 1;
    }

    //トークナイズしてパースする
    user_input = argv[1];
    token = tokenize(user_input);
    Node* node = expr();

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);

    //スタックトップに式全体の値が残っているはずなので
    //それをRAXにロードして関数からの返り値とする。
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}