#include <iostream>
#include <vector>

using namespace std;

struct TreeNode {
		int key;
		TreeNode *left, *right;
		TreeNode(int x) : key(x), left(NULL), right(NULL) {}
};

TreeNode *Insert(TreeNode *root, int x)
{
	if (root == NULL) {
		root = new TreeNode(x);
	} else if (root->key > x) {
		root->left = Insert(root->left, x);
	} else if (root->key < x) {
		root->right = Insert(root->right, x);
	}
	return root;
}

// In-order traversal
void Print(TreeNode *root)
{
	if (root == NULL)
		return;
	else {
		Print(root->left);
		cout << root->key << " ";
		Print(root->right);
	}
}

vector<TreeNode*> LongestPath(TreeNode *root)
{
	vector<TreeNode*> v, longest;
	TreeNode *last = NULL;

	if (root == NULL)
		return longest;

	v.push_back(root);
	while (!v.empty()) {
		if (v.back()->left && v.back()->left != last && v.back()->right != last) {
			v.push_back(v.back()->left);
		} else if (v.back()->right && v.back()->right != last) {
			v.push_back(v.back()->right);
		} else {
			if (v.size() > longest.size()) {
				longest = v;
			}
			last = v.back();
			v.pop_back();
		}
	}
	return longest;
}

vector<TreeNode*> LongestPathRecursion(TreeNode *root)
{
	vector<TreeNode*> v;

	if (root == NULL)
		return v;

	if (root->left == NULL && root->right == NULL) {
		v.push_back(root);
		return v;
	} else {
		vector<TreeNode*> left = LongestPathRecursion(root->left);
		vector<TreeNode*> right = LongestPathRecursion(root->right);
		if (left.size() > right.size() && right.size()) {
			left.push_back(root);
			return left;
		} else {
			right.push_back(root);
			return right;
		}
	}
}

void Delete(TreeNode *root)
{
	if (root == NULL)
		return;
	else {
		Delete(root->left);
		Delete(root->right);
		delete root;
	}
}

int main()
{
	TreeNode *root = new TreeNode(20);
	Insert(root, 15);
	Insert(root, 10);
	Insert(root, 17);
	Insert(root, 3);
	Insert(root, 12);
	Insert(root, 25);
	Insert(root, 22);
	Insert(root, 30);
	Insert(root, 33);
	Insert(root, 40);

	Print(root);
	cout << endl;
	vector<TreeNode*> longest = LongestPathRecursion(root);
	cout << "Longest path: " << longest.size() << endl;
	for (vector<TreeNode*>::iterator it = longest.begin(); it != longest.end(); it++) {
		cout << (*it)->key << ", ";
	}
	cout << endl;

	Delete(root);
}