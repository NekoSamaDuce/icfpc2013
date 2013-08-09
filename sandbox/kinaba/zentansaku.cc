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
	case NODE_E: return "e";
	case NODE_A: return "a";
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
	TreeObj(NodeType type, ARGS&&... child)
		: type_(type), child_({child...})
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

std::ostream& output_as_program(std::ostream& os, Tree t)
{
	return os << "(lambda (x) " << t << ")";
}

std::ostream& output_as_guess(std::ostream& os, const std::string& problem_id, Tree t)
{
	return output_as_program(os
		<< "{"
		<< "\"id\": \"" << problem_id << "\", "
		<< "\"program\": \"",
		t) << "\"}";
}


std::vector<Tree> generate_all(const std::set<NodeType>& ops, int size, bool in_fold, bool is_tfold = false)
{
	if(is_tfold) {
		std::vector<Tree> result;
		auto c2 = make_tree(NODE_0);
		for(int s1=1; s1<size-2-1; ++s1) {
			int s3 = size-2-s1-1;
			for(auto& c1 : generate_all(ops, s1, in_fold))
			for(auto& c3 : generate_all(ops, s3, true))
				result.emplace_back(make_tree(NODE_FOLD, c1, c2, c3));
		}
		return result;
	}

	if(size == 1) {
		if(in_fold) {
			return {
				make_tree(NODE_0),
				make_tree(NODE_1),
				make_tree(NODE_X),
				make_tree(NODE_E),
				make_tree(NODE_A),
			};
		} else {
			return {
				make_tree(NODE_0),
				make_tree(NODE_1),
				make_tree(NODE_X),
			};
		}
	}

	std::vector<Tree> result;
	for(NodeType op : ops)
	{
		if(node_arity(op) == 1)
		{
			for(auto& t : generate_all(ops, size-1, in_fold))
				result.emplace_back(make_tree(op, t));
		}
		else if(node_arity(op) == 2)
		{
			for(int s1=1; s1<size-1; ++s1) {
				int s2 = size-1-s1;
				for(auto& l : generate_all(ops, s1, in_fold))
				for(auto& r : generate_all(ops, s2, in_fold))
					result.emplace_back(make_tree(op, l, r));
			}
		}
		else if(op == NODE_IF0)
		{
			for(int s1=1; s1<size-1; ++s1)
			for(int s2=1; s1+s2<size-1; ++s1) {
				int s3 = size-1-s1-s2;
				for(auto& c1 : generate_all(ops, s1, in_fold))
				for(auto& c2 : generate_all(ops, s2, in_fold))
				for(auto& c3 : generate_all(ops, s3, in_fold))
					result.emplace_back(make_tree(op, c1, c2, c3));
			}
		}
		else if(op == NODE_FOLD && !in_fold)
		{
			for(int s1=1; s1<size-2; ++s1)
			for(int s2=1; s1+s2<size-2; ++s1) {
				int s3 = size-2-s1-s2;
				for(auto& c1 : generate_all(ops, s1, in_fold))
				for(auto& c2 : generate_all(ops, s2, in_fold))
				for(auto& c3 : generate_all(ops, s3, true))
					result.emplace_back(make_tree(op, c1, c2, c3));
			}
		}
	}
	return result;
}

std::vector<Tree> filter(const std::set<NodeType>& op, std::vector<Tree> t)
{
	t.erase(
		std::remove_if(t.begin(), t.end(), [&](Tree t){ return t->operators() != op; }),
		t.end()
	);
	return t;
}

void classify_output(const std::vector<Tree>& all)
{
	std::mt19937 rand_engine;
	rand_engine.seed(178);
	std::uniform_int_distribution<uint64_t> rand_distr(0, ~0ull);

	std::vector<uint64_t> input;
	for(int i=0; i<256; ++i)
		input.emplace_back(rand_distr(rand_engine));

	std::map<std::vector<uint64_t>, std::vector<Tree>> classified;

	for(auto& t : all)
	{
		std::vector<uint64_t> output;
		for(auto x : input)
			output.emplace_back(t->eval(x));
		classified[output].push_back(t);
	}

	std::vector<std::pair<std::vector<uint64_t>, std::vector<Tree>>> cvec(classified.begin(), classified.end());
	std::sort(cvec.begin(), cvec.end(), [&](
		const std::pair<std::vector<uint64_t>, std::vector<Tree>>& lhs,
		const std::pair<std::vector<uint64_t>, std::vector<Tree>>& rhs) {
			return lhs.second.size() < rhs.second.size();
	});

	for(auto& c : cvec) {
		std::cerr << "===== CLASS of size " << c.second.size() << " =====" << std::endl;
		for(auto& t : c.second)
			std::cerr << t << std::endl;
	}

}


int main(int argc, const char* argv[])
{
	std::string id = argv[1];
	int size = atoi(argv[2]);
	std::set<NodeType> ops;
	bool has_tfold = false;
	for(int i=3; i<argc; ++i) {
		if(argv[i] == std::string("tfold")) {
			has_tfold = true;
			ops.emplace(NODE_FOLD);
		}
		else
			ops.emplace(node_type_from_string(argv[i]));
	}

	auto all = filter(ops, generate_all(ops, size-1, false, has_tfold));

//	for(auto& t : all)
//		output_as_guess(std::cout, id, t) << std::endl;

	classify_output(all);
	std::cerr << "===== " << all.size() << " candidates generated =====" << std::endl;
}
