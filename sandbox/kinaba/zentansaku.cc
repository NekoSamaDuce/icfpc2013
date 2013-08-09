#include "util.h"
#include "tree.h"
#include <gflags/gflags.h>

std::ostream& output_as_program(std::ostream& os, Tree t)
{
	return os << "(lambda (x) " << t << ")";
}

// TODO(kinaba): be careful. it doesn't record ops.
std::map<std::tuple<int,bool,bool>, std::vector<Tree>> memo;

std::vector<Tree> generate_all(const std::set<NodeType>& ops, int size, bool in_fold, bool is_tfold = false)
{
	std::tuple<int,bool,bool> key(size,in_fold,is_tfold);
	if(memo.count(key))
		return memo[key];

	if(is_tfold) {
		std::vector<Tree> result;
		auto c2 = make_tree(NODE_0);
		for(int s1=1; s1<size-2-1; ++s1) {
			int s3 = size-2-s1-1;
			for(auto& c1 : generate_all(ops, s1, in_fold))
			for(auto& c3 : generate_all(ops, s3, true))
				result.emplace_back(make_tree(NODE_FOLD, c1, c2, c3));
		}
		return memo[key] = result;
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
				make_tree(NODE_X),
				make_tree(NODE_0),
				make_tree(NODE_1),
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
			for(int s2=1; s1+s2<size-1; ++s2) {
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
			for(int s2=1; s1+s2<size-2; ++s2) {
				int s3 = size-2-s1-s2;
				for(auto& c1 : generate_all(ops, s1, in_fold))
				for(auto& c2 : generate_all(ops, s2, in_fold))
				for(auto& c3 : generate_all(ops, s3, true))
					result.emplace_back(make_tree(op, c1, c2, c3));
			}
		}
	}
	return memo[key] = result;
}

std::vector<Tree> filter(const std::set<NodeType>& op, std::vector<Tree> t)
{
	t.erase(
		std::remove_if(t.begin(), t.end(), [&](Tree t){ return t->operators() != op; }),
		t.end()
	);
	return t;
}

std::vector<Tree> reorder(std::vector<Tree> t)
{
	std::sort(t.begin(), t.end(), [&](Tree lhs, Tree rhs) {
		return lhs->has_variable() > rhs->has_variable();
	});
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

DEFINE_int32(size, 6, "Size of the program.");
DEFINE_string(operators, "plus,not", "Comma separated list of operators.");
DEFINE_bool(classify, false, "Enable classifier");

int main(int argc, char* argv[])
{
	google::ParseCommandLineFlags(&argc, &argv, true);
	std::ios::sync_with_stdio(false);

	int size = FLAGS_size;
	std::set<NodeType> ops;
	bool has_tfold = false;
	{
		std::string param = FLAGS_operators;
		for(auto& c : param) if(c==',') c=' ';
		std::stringstream ss(param);
		for(std::string op; ss>>op; )
			if(op == std::string("tfold")) {
				has_tfold = true;
				ops.insert(NODE_FOLD);
			}
			else {
				ops.insert(node_type_from_string(op));
			}
	}

	auto all = reorder(filter(ops, generate_all(ops, size-1, false, has_tfold)));

	if(FLAGS_classify)
		classify_output(all);
	else {
		for(auto& t : all)
			output_as_program(std::cout, t) << std::endl;
	}
	std::cerr << "===== " << all.size() << " candidates generated =====" << std::endl;
}
