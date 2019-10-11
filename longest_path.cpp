#include <iostream>
#include <stack>

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

int LongestPath(TreeNode *root)
{
	stack<TreeNode*> s;
	TreeNode *last = NULL;
	int longest = 0;

	if (root == NULL)
		return longest;

	s.push(root);
	while (!s.empty()) {
		if (s.top()->left && s.top()->left != last && s.top()->right != last) {
			s.push(s.top()->left);
		} else if (s.top()->right && s.top()->right != last) {
			s.push(s.top()->right);
		} else {
			if (s.size() > longest) {
				longest = s.size();
			}
			last = s.top();
			s.pop();
		}
	}
	return longest;
}

int LongestPathRecursion(TreeNode *root)
{
	if (root == NULL)
		return 0;
	else {
		return max(LongestPathRecursion(root->left) + 1, LongestPathRecursion(root->right) + 1);
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
	Insert(root, 28);
	Insert(root, 40);

	Print(root);
	cout << endl;
	cout << "Longest path: " << LongestPath(root) << endl;
}