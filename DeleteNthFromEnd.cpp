#include <bits/stdc++.h>
#include <iostream>

using namespace std;

struct ListNode {
	int val;
    ListNode *next;
    ListNode(int x) : val(x), next(NULL) {}
};

class Solution {
public:
    ListNode* removeNthFromEnd(ListNode* head, int n) {
        ListNode *cur = head, *prev = NULL, *p = NULL;
        int count = 1;
        
        if (n==0 || !cur)
            return head;
        
        while (cur) {
            if (count < n) {
                count++;
            } else {
                if (p == NULL)
                    p = head;
				else {
					prev = p;
					p = p->next;
				}
            } 
			if (cur->next)
				cur = cur->next;
			else 
				break;
        }
		
        cout << "cur: " << cur->val << endl; 
			
        if (p) {
			cout << "p: " << p->val << endl;
			if (prev) {
				prev->next = p->next;
				free(p);
			} else {
				head = p->next;
				free(p);
			}
        }

        return head;
    }
};

int main()
{
	Solution s;
	ListNode *l = NULL, *nl;
	int n = 3;
	
	for (int i=0; i<5; i++) {
		ListNode*p= (ListNode*)calloc(1, sizeof(ListNode));
		p->val = i+1;
		p->next = l;
		l = p;
	}
	ListNode *p = l;
	while (p) {
		cout << p->val << "-->";
		p = p->next;
	}
	cout << endl;
	nl = s.removeNthFromEnd(l, n);
	p = nl;
	while (p) {
		cout << p->val << "-->";
		p = p->next;
	}
	cout << endl;
}