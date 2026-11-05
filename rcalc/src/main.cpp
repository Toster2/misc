// arbitrary precision calculator/programming language
// TODO:
//  - builtins
//  - repl
//  - put all into arena

#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <string_view>
#include <vector>
#include <immintrin.h>

#include "base.h"
#include "include/isocline.h"

#include "base.cpp"
#include "isocline/isocline.c"

using namespace std;

enum Ordering {
	LT = -1,
	EQ = 0,
	GT = 1
};

enum OperatorType {
	Prefix,
	Infix,
	Postfix,
	NotAnOperator
};

struct Nat {
	vector<U32> xs;
	Nat() : xs() {}
	Nat(U32 x) : xs()
	{
		if (x != 0) xs.push_back(x);
	}
	Nat(vector<U32> xs) : xs(xs)
	{
		normalise();
	}
	void operator+=(Nat that)
	{
		if (that.xs.size() > this->xs.size()) {
			size_t i = that.xs.size() - this->xs.size();
			while (i-- > 0) {
				xs.push_back(0);
			}
		}
		U64 c = 0;
		for (size_t i = 0; i < xs.size(); i++) {
			U64 t = (U64)(xs[i]) + (U64)(that.xs.size() > i ? that.xs[i] : 0) + c;
			c = t >> 32;
			xs[i] = (U32)t;
		}
		if (c > 0) {
			xs.push_back(c);
		}
	}
	Nat operator+(Nat that)
	{
		if (that.xs.size() > this->xs.size()) {
			return that + *this;
		}
		Nat r = *this;
		r += that;
		return r;
	}
	Nat operator*(Nat that)
	{
		vector<U32> rs(xs.size() + that.xs.size() + 1, 0);
		for (size_t i = 0; i < xs.size(); i++) {
			U64 c = 0;
			for (size_t j = 0; j < that.xs.size(); j++) {
				U64 t = (U64)xs[i] * (U64)that.xs[j] + rs[i + j] + c;
				c = t >> 32;
				rs[i + j] = (U32)t;
			}
			rs[i + that.xs.size()] = (U32)c;
		}
		while (!rs.empty() && rs.back() == 0) {
			rs.pop_back();
		}
		return Nat(rs);
	}
	void operator*=(Nat that)
	{
		auto r = *this * that;
		*this = r;
	}
	void operator-=(Nat that)
	{
		assert(cmp(that) != LT);
		for (size_t i = 0; i < that.xs.size(); i++) {
			if (xs[i] >= that.xs[i]) {
				xs[i] -= that.xs[i];
				continue;
			} else {
				xs[i] = (UINT32_MAX - (that.xs[i] - xs[i])) + 1;
				for (size_t j = i + 1; j < xs.size(); j++) {
					if (xs[j] == 0) {
						xs[j] = UINT32_MAX;
					} else {
						xs[j] -= 1;
						break;
					}
				}
			}
		}
		normalise();
	}
	void lshift1()
	{
		size_t sz = xs.size();
		if (!xs.empty() && (xs.back() & (U32(1) << 31)) > 0) {
			xs.push_back(1);
		}
		U32 c = 0;
		for (size_t i = 0; i < sz; i++) {
			U32 c1 = !!(xs[i] & (U32(1) << 31));
			xs[i] <<= 1;
			xs[i] |= c;
			c = c1;
		}
	}
	Ordering cmp(Nat d)
	{
		if (xs.size() > d.xs.size()) return GT;
		if (xs.size() < d.xs.size()) return LT;
		for (int i = (int)xs.size() - 1; i >= 0; i--) {
			if (xs[i] > d.xs[i]) return GT;
			if (xs[i] < d.xs[i]) return LT;
		}
		return EQ;
	}
	void normalise()
	{
		while (!xs.empty() && xs.back() == 0) {
			xs.pop_back();
		}
	}
	bool divmod(Nat d, Nat &q, Nat &r)
	{
		q.xs.clear();
		r.xs.clear();
		if (d.xs.empty()) {
			return true;
		}
		if (xs.empty()) return false;
		q.xs.assign(xs.size(), 0);
		constexpr int bw = sizeof(U32) * 8;
		for (int i = xs.size() * bw - 1; i >= 0; i--) {
			r.lshift1();
			if (r.xs.empty()) {
				r.xs.push_back(0);
			}
			r.xs[0] |= (xs[i / bw] >> (i % bw)) & U32(1);
			if (r.cmp(d) != LT) {
				r -= d;
				q.xs[i / bw] |= (U32(1) << (i % bw));
			}
		}
		q.normalise();
		r.normalise();
		return false;
	}
	void gcd(Nat that)
	{
		while (!that.xs.empty()) {
			Nat q, r;
			divmod(that, q, r);
			swap(*this, that);
			that = r;
		}
	}

	void prin()
	{
		string s;
		Nat t = *this, q, r;
		if (xs.empty()) {
			putchar('0');
			return;
		}
		while (!xs.empty()) {
			divmod(10, q, r);
			*this = q;
			if (r.xs.empty()) {
				s.push_back('0');
			} else {
				s.push_back(r.xs[0] + '0');
			}
		}
		*this = t;
		for (size_t i = s.size(); i > 0; i--) {
			putchar(s[i - 1]);
		}
	}
};

struct Rat {
	bool sign = false;
	Nat num, denom;
	Rat() {}
	Rat(Nat n, Nat d) : num(n), denom(d) {}
	Rat(U32 n, U32 d) : num(n), denom(d) {}

	void normalise()
	{
		assert(!denom.xs.empty());
		Nat q, r, d = num;
		d.gcd(denom);
		num.divmod(d, q, r);
		assert(r.xs.empty());
		num = q;
		num.normalise();
		denom.divmod(d, q, r);
		assert(r.xs.empty());
		denom = q;
		denom.normalise();
	}
	void operator+=(Rat that)
	{
		if (sign != that.sign) {
			// x + (-y)
			if (that.sign) {
				that.sign = 0;
				*this -= that;
			} else {
				// (-x) + y = -(x - y)
				sign = 0;
				*this -= that;
				sign = !sign;
			}
		} else {
			num *= that.denom;
			num += that.num * denom;
			denom *= that.denom;
			normalise();
		}
	}
	// R-
	void operator-=(Rat that)
	{
		if (sign != that.sign) {
			if (that.sign) {
				that.sign = 0;
				*this += that;
			} else {
				sign = 0;
				*this += that;
				sign = !sign;
			}
			return;
		}
		switch (cmp(that)) {
		case EQ:
			num = 0;
			denom = 1;
			sign = 0;
			break;
		case LT:
			that -= *this;
			*this = that;
			sign = !sign;
			break;
		case GT:
			num *= that.denom;
			num -= that.num * denom;
			denom *= that.denom;
			normalise();
			break;
		}
	}
	void operator*=(Rat that)
	{
		num *= that.num;
		denom *= that.denom;
		sign = that.sign ? !sign : sign;
		normalise();
	}
	bool div(Rat that)
	{
		if (that.iszero()) return true;
		num *= that.denom;
		denom *= that.num;
		sign = that.sign ? !sign : sign;

		normalise();
		return false;
	}
	bool idiv(Rat that)
	{
		Nat q, r;
		if (this->div(that)) return true;
		if (num.divmod(denom, q, r)) return true;
		num = q;
		denom = 1;
		return false;
	}
	bool mod(Rat that)
	{
		Nat q, r;
		if (this->div(that)) return true;
		if (num.divmod(denom, q, r)) return true;
		num = r;
		denom = 1;
		return false;
	}
	bool iszero()
	{
		return num.xs.empty();
	}
	Ordering cmp(Rat y)
	{
		int up = 1;
		if (iszero() && y.iszero()) {
			return EQ;
		}
		if (sign != y.sign) {
			if (y.sign) {
				return GT;
			} else {
				return LT;
			}
		}
		if (sign == 1) up = -1;
		Rat x;
		x = *this;
		x.num *= y.denom;
		y.num *= x.denom;
		return Ordering(up * x.num.cmp(y.num));
	}
	void prin()
	{
		if (iszero()) {
			putchar('0');
		} else {
            if (sign) putchar('-');
			num.prin();
		}
		if (denom.cmp(1) != EQ) {
			putchar('/');
			denom.prin();
		}
		putchar('\n');
	}
};

enum TokenKind {
    TOK_ERR,
	TOK_EOF,
	TOK_ID,
	TOK_NUM,
	TOK_OP,
    TOK_LET,
};

constexpr Byte CH_SOF = UINT8_MAX - 1;
constexpr Byte CH_EOF = UINT8_MAX;

bool is_ident(Byte c)
{
	return IS_BETWEEN(c, 'a', 'z') || IS_BETWEEN(c, 'A', 'Z');
}

bool is_num(Byte c)
{
	return IS_BETWEEN(c, '0', '9');
}

bool is_ident1(Byte c)
{
	return is_ident(c) || is_num(c);
}

bool whitespace(Byte c)
{
	return strchr(" \f\n\t\r\v\b", c);
}

bool isop(Byte c)
{
	return strchr("!()+-*/^!\\%=", c);
}

struct Location {
	Size pos = 0, len = 0;

	Location cat(Location that)
	{
		Location r;
		if (this->pos < that.pos) {
			r.pos = this->pos;
			r.len = (that.pos - this->pos) + that.len;
		} else {
			r.pos = that.pos;
			r.len = (this->pos - that.pos) + this->len;
		}
		return r;
	}
};

struct ErrCtx {
	Byte msg[256];
	Size msglen = 0;
	Location where;

	void setmsg(const char *s, ...)
	{
		va_list ap;
		va_start(ap, s);
		msglen = vsnprintf((char *)msg, 256, s, ap);
		va_end(ap);
	}

	void prin(string_view line)
	{
		fprintf(stderr, "Error:%td-%td: %.*s\n", where.pos + 1, where.pos + where.len, (int)msglen, msg);
		fprintf(stderr, "  %.*s\n", FMT(line));
		for (int i = 0; i < 2 + where.pos; i++) {
			fputc(' ', stderr);
		}
		for (int i = 0; i < where.len; i++) {
			fputc('^', stderr);
		}
		fputc('\n', stderr);
	}
};

Rat read_num(string_view s)
{
	bool sub = false;
	Rat r(0, 1);
	for (Byte c : s) {
		if (c == '.') {
			sub = true;
			continue;
		}
		r.num *= 10;
		r.num += c - '0';
		if (sub)
			r.denom *= 10;
	}

	r.normalise();

	return r;
}

// :scanner
struct Scanner {
	string_view src;
	Size column = 0;
	Size src_pos = 0;
	Size tok_beg = -1;
	Size tok_len = 0;
	Byte ch = CH_SOF;
	ErrCtx errctx;

	Byte peek_char(int n = 0)
	{
		if (ch == CH_SOF) {
			ch = advance();
		} else if (src_pos > (Size)src.length()) {
			ch = CH_EOF;
			return ch;
		}
		Byte c = ch;
		if (n > 0) {
			Scanner prev_s = *this;
			while (n-- > 0) {
				c = peek_char();
				if (c != CH_EOF) {
					advance();
				}
			}
			*this = prev_s;
		}
		return c;
	}

	Byte advance()
	{
		if (src_pos > (Size)src.length()) {
			return CH_EOF;
		}
		// hacky
		Byte c = ch = src_pos == (Size)src.length() ? (src_pos++, ' ') : src[src_pos++];
		if (c == '\n') {
			column = 0;
		}
		return c;
	}

	TokenKind scan()
	{
		Byte c = peek_char();
		tok_beg = -1;
		tok_len = 0;
		if (c == CH_EOF) return TOK_EOF;
		while (true) {
			while (whitespace(c)) {
				c = advance();
			}
			tok_beg = src_pos - 1;
			if (is_ident(c)) {
				for (c = advance(); is_ident1(c); c = advance());
				tok_len = src_pos - tok_beg - (c != CH_EOF);
                if (token() == "let") return TOK_LET;
				return TOK_ID;
			} else if (is_num(c)) {
				bool dot = false, afterdot = false;
				for (c = advance(); (c == '.' && !dot) || is_num(c); c = advance()) {
					if (dot) afterdot = true;
					if (c == '.') dot = true;
				}
				tok_len = src_pos - tok_beg - (c != CH_EOF);
				if (dot && !afterdot) {
					errctx.setmsg("no digits after '.'");
					errctx.where.pos = tok_beg;
					errctx.where.len = tok_len;
					return TOK_ERR;
				}
				return TOK_NUM;
			} else if (isop(c)) {
				Byte o = advance();
				switch (c) {
				case '=':
				case '(':
				case ')':
				case '\\':
				case '+':
				case '*':
				case '%':
				case '^':
				case '!':
					tok_len = 1;
					break;
				case '-':
					if (o == '>') { // ->
						advance();
						tok_len = 2;
					} else {
						tok_len = 1;
					}
					break;
				case '/':
					if (o == '/') { // //
						advance();
						tok_len = 2;
					} else {
						tok_len = 1;
					}
					break;
				default:
					errctx.setmsg("unexpected character: '%c'", c);
					errctx.where.pos = tok_beg;
					errctx.where.len = 1;
					return TOK_ERR;
				}
				return TOK_OP;
			} else if (c == CH_EOF) {
				return TOK_EOF;
			} else {
				errctx.setmsg("unexpected character: '%c'", c);
				errctx.where.pos = tok_beg;
				errctx.where.len = 1;
				return TOK_ERR;
			}

			// TODO...
		}
	}

	TokenKind peek(string_view &t, Location &loc)
	{
		Scanner s = *this;
		TokenKind tk = s.scan();
		loc = s.location();
		t = s.token();
		return tk;
	}

	string_view token()
	{
		return tok_beg < 0 ? Str(0, 0) : src.substr(tok_beg, tok_len);
	}

	Location location()
	{
		return Location{tok_beg, tok_len};
	}
};

enum AstKind : U8 {
	Lam,
	App,
	Val,
	Var,
	Intr
};

enum IntrKind : U8 {

};

struct Ast;
struct Value;

using Env = vector<pair<string_view, Value>>;

bool is_calc_identifier_char(const char *s, long len)
{
	if (len != 1) return false;
	Byte c = (Byte)s[0];
	return is_ident(c) || is_num(c);
}

enum ValueKind : U8 {
	VNum,
	VFun
};
struct Value {
	ValueKind kind = VNum;
	Rat num;
	Ast *fun = 0;
	Env env;
	Location loc;
};

void complete_scoped_variables(ic_completion_env_t *cenv, const char *prefix)
{
	Env *xs = static_cast<Env *>(ic_completion_arg(cenv));
	if (xs == nullptr) return;

	string_view word(prefix);
	vector<string_view> emitted;
	for (auto it = xs->rbegin(); it != xs->rend(); ++it) {
		string_view name = it->first;
		bool shadowed = false;
		for (string_view previous : emitted) {
			if (previous == name) {
				shadowed = true;
				break;
			}
		}
		if (shadowed) continue;
		emitted.push_back(name);

		if (name.size() < word.size() || name.compare(0, word.size(), word) != 0)
			continue;

		string completion(name.data(), name.size());
		if (!ic_add_completion(cenv, completion.c_str())) return;
	}
}

void complete_variables(ic_completion_env_t *cenv, const char *prefix)
{
	ic_complete_word(cenv, prefix, &complete_scoped_variables,
	                 &is_calc_identifier_char);
}

static void highlighter(ic_highlight_env_t* henv, const char* input, void* arg) {
    (void)arg;
    Scanner s;
    s.src = string_view(input);
    while (true) {
        TokenKind tk = s.scan();
        switch (tk) {
		case TOK_ERR:
		case TOK_EOF:
            return;
		case TOK_ID:
            break;
		case TOK_NUM:
            ic_highlight(henv, s.tok_beg, s.tok_len, "number");
            break;
		case TOK_OP:
            break;
		case TOK_LET:
            ic_highlight(henv, s.tok_beg, s.tok_len, "keyword");
            break;
		}
    }
}

// Lam var l
// App l r
// Val val
// Intr1 l
// Intr2 l r
struct Ast {
	string_view var;
	Value val;
	Ast *l;
	Ast *r;
	AstKind kind;
	Location loc;
	Byte intr;
};

// -(sqrt x) ?
// (-sqrt) x ?
// sqrt (-x) x

Byte parse_intr(string_view s, OperatorType opt)
{
	if (opt == Prefix) {
		if (s == "+") return 'i';
		if (s == "-") return '_';
	} else if (opt == Infix) {
		if (s == "+") return '+';
		if (s == "-") return '-';
		if (s == "*") return '*';
		if (s == "/") return '/';
		if (s == "//") return ':';
		if (s == "%") return ':';
		if (s == "^") return '^';
	} else if (opt == Postfix) {
		if (s == "!") return '!';
	} else {
	}
	return 0;
}

bool eval_intr(Value *ret, Byte f, Value x, Value y, ErrCtx &ctx)
{
	switch (f) {
	case 'i':
		*ret = x;
		return false;
	}

	if (x.kind != VNum) {
		ctx.setmsg("expected a number");
		ctx.where = x.loc;
		return true;
	}

	switch (f) {
	case '_':
		*ret = x;
		ret->num.sign = !ret->num.sign;
		return false;
	case '!':
		if (x.num.denom.cmp(1) != EQ || x.num.sign) {
			ctx.setmsg("factorial expected a natural number");
			ctx.where = x.loc;
			return true;
		}
		Nat r = 1, i = 1;
		for (; i.cmp(x.num.num) != GT; i += 1) {
			r *= i;
		}
		ret->kind = VNum;
		ret->num.num = r;
		ret->num.denom = 1;
		return false;
	}
	*ret = x;

	if (y.kind != VNum) {
		ctx.setmsg("expected a number");
		ctx.where = y.loc;
		return true;
	}
	switch (f) {
	case '+':
		ret->num += y.num;
		return false;
	case '-':
		ret->num -= y.num;
		return false;
	case '*':
		ret->num *= y.num;
		return false;
	case '/':
		if (ret->num.div(y.num)) {
			ctx.setmsg("division by zero");
			ctx.where = y.loc;
			return true;
		}
		return false;
	case ':':
		if (ret->num.idiv(y.num)) {
			ctx.setmsg("division by zero");
			ctx.where = y.loc;
			return true;
		}
		return false;
	case '%':
		if (ret->num.mod(y.num)) {
			ctx.setmsg("modulus of zero");
			ctx.where = y.loc;
			return true;
		}
		return false;
	case '^':
		if (y.num.iszero() && x.num.iszero()) {
			ctx.setmsg("0^0");
			ctx.where = x.loc.cat(y.loc);
			return true;
		}
		if (y.num.denom.cmp(1) != EQ) {
			ctx.setmsg("non-integer power");
			ctx.where = y.loc;
			return true;
		}
		Nat r = 1, i = 0;
		for (; i.cmp(y.num.num) == LT; i += 1) {
			r *= x.num.num;
		}
		if (y.num.sign) {
			ret->num.num = 1;
			ret->num.denom = r;
		} else {
			ret->num.num = r;
			ret->num.denom = 1;
		}
		return false;
	}
	return true;
}

namespace parser {
bool expr(Scanner &sc, Ast *&lhs, int min_bp);

// clang-format off
bool infix_bp(char op, int &lbp, int &rbp)
{
	switch (op) {
	case '+': case '-': lbp = 12; rbp = 13; break;
	case '*': case '/': case '%': lbp = 14; rbp = 15; break;
	case '^': lbp = 16; rbp = 17; break;
	default: return true;
	}
	return false;
}

bool prefix_bp(char op, int &rbp)
{
	switch (op) {
	case '+': case '-': rbp = 15; break;
	default: return true;
	}
	return false;
}

bool postfix_bp(char op, int &rbp)
{
	switch (op) {
	case '!': rbp = 17; break;
	default: return true;
	}
	return false;
}
// clang-format on

string_view frompfx(string_view s)
{
	static const string_view minus = "_";
	static const string_view plus = "id";
	if (s == "-") return minus;
	if (s == "+") return plus;
	return s;
}

bool term(Scanner &sc, Ast *&ast)
{
	int rbp;
	Ast *rhs;
	string_view pfx;
	Location pfxloc;
	Scanner sc2 = sc;
	switch (sc.scan()) {
    case TOK_ERR: /* fallthrough */
	case TOK_EOF:
    case TOK_LET:
		return true;
	case TOK_ID:
		ast->kind = Var;
		ast->var = sc.token();
		ast->loc = sc.location();
		return false;
	case TOK_NUM:
		ast->kind = Val;
		ast->val.kind = VNum;
		ast->val.num = read_num(sc.token());
		ast->val.loc = sc.location();
		ast->loc = sc.location();
		return false;
	case TOK_OP:
		pfx = sc.token();
		pfxloc = sc.location();
		if (pfx == "\\") {
			// NOTE: JEBAC C++ KURWA MAC
			Ast **n = &ast;
			TokenKind tk;
			bool happened = false;
			while ((tk = sc.scan()) == TOK_ID) {
				(*n)->kind = Lam;
				(*n)->var = sc.token();
				(*n)->loc = sc.location();
				(*n)->l = new Ast;
				n = &(*n)->l;
				happened = true;
			}
            if (tk == TOK_ERR) {
                return true;
            }
			if (tk != TOK_OP || sc.token() != "->") {
				sc.errctx.setmsg("expected '->' in lambda, got '%.*s'", sc.token());
				sc.errctx.where = sc.location();
				return true;
			}
			if (!happened) {
				sc.errctx.setmsg("missing arguments to lambda", sc.token());
				sc.errctx.where = sc.location();
				return true;
			}
			if (expr(sc, *n, 0)) return true;
		} else if (pfx == "(") {
			if (expr(sc, ast, 0)) return true;
			sc.scan();
			if (sc.token() != ")") {
				sc.errctx.setmsg("unclosed parenthesis");
				sc.errctx.where = pfxloc;
				return true;
			}
		} else {
			if (prefix_bp(pfx[0], rbp)) {
				sc = sc2;
				return true;
			}
			rhs = new Ast;
			if (expr(sc, rhs, rbp)) return true;
			Byte i = parse_intr(pfx, Prefix);
			if (i) {
				ast->kind = Intr;
				ast->intr = i;
				ast->l = rhs;
				ast->r = new Ast;
				ast->r->kind = Val;
			} else {
				ast->kind = App;
				ast->l = new Ast;
				ast->l->kind = Var;
				ast->l->var = pfx;
				ast->l->loc = pfxloc;
				ast->r = rhs;
				ast->loc = ast->l->loc.cat(ast->r->loc);
			}
		}
		return false;
	}
	return true;
}

bool opexpr(Scanner &sc, Ast *&ast)
{
	Ast *z = new Ast, *x;
	string_view s;
	TokenKind tk;
	Location loc;
	if (term(sc, z)) return true;
	while (true) {
		Scanner sc2 = sc;
		tk = sc2.scan();
		s = sc2.token();
		loc = sc2.location();
        if (tk == TOK_ERR) {
            sc = sc2;
            return true;
        }
		if (tk == TOK_EOF) {
			sc = sc2;
			break;
		}
		if (!(tk == TOK_OP && s == "(") && tk != TOK_ID && tk != TOK_NUM) {
			break;
		}
		x = new Ast;
		if (term(sc, x)) {
			break;
		}
		Ast *n = new Ast;
		n->kind = App;
		n->l = z;
		n->r = x;
		n->loc = n->l->loc.cat(n->r->loc);
		z = n;
	}
	*ast = *z;
	delete z;
	return false;
}

bool expr(Scanner &sc, Ast *&lhs, int min_bp = 0)
{
	int lbp, rbp;
	Str pfx;
	Ast *rhs, *n;
	if (opexpr(sc, lhs)) return true;

	while (true) {
		string_view op;
		Location oploc;
		TokenKind opt = sc.peek(op, oploc);
        if (opt == TOK_ERR) return true;
		if (opt == TOK_EOF) break;
		else if (opt != TOK_OP) {
			sc.errctx.setmsg("'%.*s' is not an operator", FMT(op));
			sc.errctx.where = oploc;
			return true;
		}
		if (!postfix_bp(op[0], lbp)) {
			if (lbp < min_bp) break;
			if (sc.scan() == TOK_ERR) return true;
			Byte i = parse_intr(op, Postfix);
			n = new Ast;
			if (i) {
				n->kind = Intr;
				n->intr = i;
				n->loc = lhs->loc;
				n->l = lhs;
				n->r = new Ast;
				n->r->kind = Val;
			} else {
				n->kind = App;
				n->l = new Ast;
				n->l->kind = Var;
				n->l->var = op;
				n->l->loc = oploc;
				n->r = lhs;
				n->loc = n->l->loc.cat(n->r->loc);
			}
			lhs = n;
			continue;
		}
		if (infix_bp(op[0], lbp, rbp)) break;
		if (lbp < min_bp) break;
		if (sc.scan() == TOK_ERR) return true;
		rhs = new Ast;
		if (expr(sc, rhs, rbp)) return true;
		Byte i = parse_intr(op, Infix);
		if (i) {
			n = new Ast;
			n->kind = Intr;
			n->intr = i;
			n->loc = lhs->loc.cat(rhs->loc);
			n->l = lhs;
			n->r = rhs;
			lhs = n;
		} else {
			n = new Ast;
			n->kind = App;
			n->l = new Ast;
			n->l->kind = App;
			n->l->l = new Ast;
			n->l->l->kind = Var;
			n->l->l->var = op;
			n->l->l->loc = oploc;
			n->l->r = lhs;
			n->l->loc = n->l->l->loc.cat(n->l->r->loc);
			n->r = rhs;
			n->loc = n->l->loc.cat(n->r->loc);
			lhs = n;
		}
	}
	return false;
}

bool defun(Scanner &sc, string_view &name, Ast *&ast) {
    TokenKind tk;
    if ((tk = sc.scan()) != TOK_ID) {
        if (tk != TOK_ERR) {
            sc.errctx.setmsg("expected variable name");
            sc.errctx.where = sc.location();
        }
        return true;
    }
    name = sc.token();
    Ast **n = &ast;

    while (true) {
        tk = sc.scan();
        if (tk == TOK_ERR) return true;
        if (tk == TOK_OP && sc.token() == "=") break;
        if (tk != TOK_ID) {
            sc.errctx.setmsg("expected '=' or function args");
            sc.errctx.where = sc.location();
            return true;
        }

        (*n)->kind = Lam;
        (*n)->var = sc.token();
        (*n)->l = new Ast;
        n = &(*n)->l;
    }
    return expr(sc, *n);
}


} // namespace parser

void prin(Ast *ast, int tab = 0)
{
	printf("%*c %td-%td ", tab, ' ', ast->loc.pos + 1, ast->loc.pos + ast->loc.len);
	switch (ast->kind) {
	case Lam:
		printf("Lam %.*s:\n", FMT(ast->var));
		prin(ast->l, tab += 2);
		break;
	case App:
		printf("App:\n");
		prin(ast->l, tab + 2);
		prin(ast->r, tab + 2);
		break;
	case Val:
		switch (ast->val.kind) {
		case VNum:
			ast->val.num.prin();
			break;
		case VFun:
			prin(ast->val.fun, tab + 2);
			break;
		}
		break;
	case Var:
		printf("%.*s\n", FMT(ast->var));
		break;
	case Intr:
		printf("Intr %c:\n", ast->intr);
		prin(ast->l, tab + 2);
		if (ast->r)
			prin(ast->r, tab + 2);
		else
			printf("{}\n");
		break;
	}
}

void prin(Value &v)
{
	switch (v.kind) {
	case VNum:
		v.num.prin();
		break;
	case VFun:
		prin(v.fun);
		break;
	}
}

bool eval(Value *ret, Ast *ast, vector<pair<string_view, Value>> &xs, ErrCtx &ctx)
{
	switch (ast->kind) {
	case Lam: {
		ret->kind = VFun;
		ret->fun = ast;
		ret->env = xs;
		ret->loc = ast->loc;
		return false;
	} break;
	case App: {
		Value f, x;
		if (eval(&f, ast->l, xs, ctx)) return true;
		f.loc = ast->l->loc;
		if (f.kind != VFun) {
			ctx.setmsg("expected a function");
			ctx.where = ast->loc;
			return true;
		}
		assert(f.kind == VFun);
		if (eval(&x, ast->r, xs, ctx)) return true;
		x.loc = ast->r->loc;
		Env nenv = f.env;
		nenv.emplace_back(f.fun->var, x);
		return eval(ret, f.fun->l, nenv, ctx);
	} break;
	case Intr: {
		Value x, y;
		if (eval(&x, ast->l, xs, ctx)) return true;
		if (eval(&y, ast->r, xs, ctx)) return true;
		return eval_intr(ret, ast->intr, x, y, ctx);
	} break;
	case Val: {
		*ret = ast->val;
		ret->loc = ast->loc;
		return false;
	} break;
	case Var: {
		for (int i = xs.size() - 1; i >= 0; i--) {
			if (xs[i].first == ast->var) {
				*ret = xs[i].second;
				return false;
			}
		}
		ctx.setmsg("variable not in scope: %.*s", FMT(ast->var));
		ctx.where = ast->loc;
		return true;
	} break;
	}
	ctx.setmsg("eval broke somehow?? %d", ast->kind);
	ctx.where = ast->loc;
	return true;
}

int main(int argc, char **argv)
{
	ic_set_history(0, -1);
	char *input;
	Env xs;
	ic_set_default_completer(&complete_variables, &xs);
    ic_set_default_highlighter(highlighter, 0);
	while ((input = ic_readline("rcalc")) != NULL) { // ctrl+d or errors return NULL
		string_view inp(input), name;
		Scanner s, sc;
		s.src = inp;
		Ast *ast = new Ast;
		sc = s;
        ErrCtx ctx;
		Value v;
        if (sc.scan() == TOK_LET) {
            if (parser::defun(sc, name, ast)) goto e;
            // prin(ast);
            if(eval(&v, ast, xs, ctx)) {
				ctx.prin(s.src);
            } else {
                xs.emplace_back(name, v);
                // prin(v);
            }
            continue;
        }

		if (parser::expr(s, ast)) {
        e:
			if (s.errctx.msglen == 0) {
				s.errctx.where = s.location();
				s.errctx.setmsg("unfinished expression");
			}
			s.errctx.prin(s.src);
		} else {
			// prin(ast);
			if (eval(&v, ast, xs, ctx)) {
				ctx.prin(s.src);
			} else {
				prin(v);
			}
		}
	}
	return 0;
}
