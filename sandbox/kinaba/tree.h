#pragma once
#include "util.h"

enum NodeType
{
	// 0-ary
	NODE_0,
	NODE_1,
	NODE_X,  // input variable
	NODE_E,  // first parameter of fold ("E"lement)
	NODE_A,  // second parameter of fold ("A"ccumulator)
	// 1-ary
	NODE_NOT,
	NODE_SHL1,
	NODE_SHR1,
	NODE_SHR4,
	NODE_SHR16,
	// 2-ary
	NODE_AND,
	NODE_OR,
	NODE_XOR,
	NODE_PLUS,
	// 3-ary
	NODE_IF0,
	NODE_FOLD,  // NOTE: sometimes FOLD is assumed be the last enum
};

int node_arity(NodeType type)
{
	switch(type)
	{
	case NODE_0:
	case NODE_1:
	case NODE_X:
	case NODE_E:
	case NODE_A:
		return 0;
	case NODE_NOT:
	case NODE_SHL1:
	case NODE_SHR1:
	case NODE_SHR4:
	case NODE_SHR16:
		return 1;
	case NODE_AND:
	case NODE_OR:
	case NODE_XOR:
	case NODE_PLUS:
		return 2;
	case NODE_IF0:
	case NODE_FOLD:
		return 3;
	}
	NOTREACHED();
}

const char* node_name(NodeType type)
{
	switch(type)
	{
	case NODE_0: return "0";
	case NODE_1: return "1";
	case NODE_X: return "x";
	case NODE_E: return "y";
	case NODE_A: return "z";
	case NODE_NOT:   return "not";
	case NODE_SHL1:  return "shl1";
	case NODE_SHR1:  return "shr1";
	case NODE_SHR4:  return "shr4";
	case NODE_SHR16: return "shr16";
	case NODE_AND:  return "and";
	case NODE_OR:   return "or";
	case NODE_XOR:  return "xor";
	case NODE_PLUS: return "plus";
	case NODE_IF0:  return "if0";
	case NODE_FOLD: return "fold";
	}
	NOTREACHED();
}

NodeType node_type_from_string(const std::string& str)
{
	for(int i=0; i<=NODE_FOLD; ++i) {
		NodeType t = NodeType(i);
		if(node_name(t) == str)
			return t;
	}
	NOTREACHED();
}

typedef std::shared_ptr<class TreeObj> Tree;

class TreeObj
{
public:
	template<typename... ARGS>
	TreeObj(NodeType type, ARGS&&... child) : type_(type), child_({child...})
	{
		assert(node_arity(type_) == child_.size());
	}

	NodeType type() const { return type_; }
	Tree child(int i) const { return child_.at(i); }

	friend std::ostream& operator<<(std::ostream& os, Tree t)
	{
		switch(t->type())
		{
		case NODE_0:
		case NODE_1:
			os << node_name(t->type());
			break;
		case NODE_X:
			os << "x";
			break;
		case NODE_E:
			os << "e";
			break;
		case NODE_A:
			os << "a";
			break;
		case NODE_NOT:
		case NODE_SHL1:
		case NODE_SHR1:
		case NODE_SHR4:
		case NODE_SHR16:
			os << "(" << node_name(t->type()) << " " << t->child(0) << ")";
			break;
		case NODE_AND:
		case NODE_OR:
		case NODE_XOR:
		case NODE_PLUS:
			os << "(" << node_name(t->type()) << " " << t->child(0) << " " << t->child(1) << ")";
			break;
		case NODE_IF0:
			os << "(" << node_name(t->type()) << " "
				<< t->child(0) << " " << t->child(1) << " " << t->child(2) << ")";
			break;
		case NODE_FOLD:
			os << "(" << node_name(t->type()) << " "
				<< t->child(0) << " " << t->child(1) << " "
				<< "(lambda (e a) " << t->child(2) << "))";
			break;
		}
		return os;
	}

	std::set<NodeType> operators() const
	{
		std::set<NodeType> result;

		std::function<void(const TreeObj&)> rec;
		rec = [&](const TreeObj& t) {
			if(node_arity(t.type()) > 0)
				result.emplace(t.type());
			for(auto& ch : t.child_)
				rec(*ch);
		};

		rec(*this);
		return result;
	}

	uint64_t eval(uint64_t x, uint64_t e=0, uint64_t a=0)
	{
		switch(type())
		{
		case NODE_0:
			return 0;
		case NODE_1:
			return 1;
		case NODE_X:
			return x;
		case NODE_E:
			return e;
		case NODE_A:
			return a;
		case NODE_NOT:
			return ~ child(0)->eval(x,e,a);
		case NODE_SHL1:
			return child(0)->eval(x,e,a) << 1;
		case NODE_SHR1:
			return child(0)->eval(x,e,a) >> 1;
		case NODE_SHR4:
			return child(0)->eval(x,e,a) >> 4;
		case NODE_SHR16:
			return child(0)->eval(x,e,a) >> 16;
		case NODE_AND:
			return child(0)->eval(x,e,a) & child(1)->eval(x,e,a);
		case NODE_OR:
			return child(0)->eval(x,e,a) | child(1)->eval(x,e,a);
		case NODE_XOR:
			return child(0)->eval(x,e,a) ^ child(1)->eval(x,e,a);
		case NODE_PLUS:
			return child(0)->eval(x,e,a) + child(1)->eval(x,e,a);
		case NODE_IF0:
			return child(0)->eval(x,e,a)==0 ? child(1)->eval(x,e,a) : child(2)->eval(x,e,a);
		case NODE_FOLD:
			uint64_t seq = child(0)->eval(x,e,a);
			a = child(1)->eval(x,e,a);
			for(int i=0; i<64; i+=8) {
				e = (seq >> i) & 0xff;
				a = child(2)->eval(x,e,a);
			}
			return a;
		}
		NOTREACHED();
	}

private:
	NodeType     type_;
	std::vector<Tree> child_;
};

template<typename... ARGS>
Tree make_tree(ARGS&&... args)
{
	return std::make_shared<TreeObj>(args...);
}
