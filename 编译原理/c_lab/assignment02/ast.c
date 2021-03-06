
/************************************************************/
/*      copyright hanfei.wang@gmail.com                     */
/*             2019.09.10                                   */
/************************************************************/

#include "ast.h"
EXPTAB exptab[HASHSIZE] = {NULL}; /* 符号表 */

#define realloc(X, N) ((X) == NULL? malloc(N): realloc((X), (N)))

void *safe_allocate(unsigned Bytes) {
   void *X = malloc(Bytes);
   if (X == NULL) printf("memory exhasuted.\n"), exit(1);
   return X;
}

void *safe_reallocate(void *X, unsigned Bytes) {
   X = realloc(X, Bytes);
   if (X == 0) printf("memory exhasuted\n"), exit(1);
   return X;
}




int hash(char *s)
{
  unsigned int hv = 7, len = strlen(s);
  int i;
  for (i = 0; i < len; i++) {
    hv = hv*31 + s[i];
  }
  return (int) (hv % HASHSIZE) ;  
}
 

AST_PTR insert(AST_PTR exp)
{
  int hv = exp->hash;
  
  EXPTAB new = (EXPTAB) safe_allocate(sizeof(*new));
  new -> next = exptab[hv];
  new -> exp = exp;
  exptab[hv] = new;

  return exp;
}


AST_PTR lookup(char *exp_string)
{
  int hv = hash(exp_string);
  
  EXPTAB t = exptab[hv];

  if (t == NULL) return NULL;
  
  while (t != NULL) {
    if (strcmp(exp_string, t -> exp -> exp_string) == 0) {
      break;
    }
    t = t -> next;
  }
  if (t == NULL) return NULL;
  return t -> exp;
}


AST_PTR mkLeaf(char c)
{
  AST_PTR tree_tmp;
  char exp[2] = "x";
  exp[0] = c;
  tree_tmp = lookup(exp);
  
  if (tree_tmp != NULL) return tree_tmp;
  
  tree_tmp = (AST_PTR) safe_allocate(sizeof(*tree_tmp));
  tree_tmp->op = Alpha;
  tree_tmp->exp_string = strdup(exp);
  tree_tmp->hash=hash(exp);
  tree_tmp->nullable=0;
  tree_tmp->lf = NULL;
  tree_tmp->state = -1;
  tree_tmp->lchild = NULL;
  tree_tmp->rchild = NULL;
  return insert(tree_tmp);
}

AST_PTR mkEpsilon (void) 
{
  AST_PTR tree_tmp; 

  tree_tmp = lookup ("ε");
  
  if (tree_tmp != NULL) return tree_tmp;

  tree_tmp = (AST_PTR) safe_allocate(sizeof(*tree_tmp));
  tree_tmp->op = Epsilon;
  tree_tmp->exp_string = strdup("ε");
  tree_tmp->hash = hash(tree_tmp->exp_string);
  tree_tmp->nullable = 1;
  tree_tmp->state = -1;
  tree_tmp->lf = NULL;
  tree_tmp->lchild = NULL;
  tree_tmp->rchild = NULL;
  return insert(tree_tmp);
}

AST_PTR mkEmpty (void) 
{
  AST_PTR tree_tmp;

  tree_tmp = lookup ("ϕ");
  
  if (tree_tmp != NULL) return tree_tmp;

  tree_tmp = (AST_PTR ) safe_allocate(sizeof(*tree_tmp));
  tree_tmp->op = Empty;
  tree_tmp->exp_string = strdup("ϕ");
  tree_tmp->hash = hash(tree_tmp->exp_string);
  tree_tmp->nullable = 0;
  tree_tmp->lf = NULL;
  tree_tmp->state = -1;
  tree_tmp->lchild = NULL;
  tree_tmp->rchild = NULL;
  return insert(tree_tmp);
}



AST_PTR mkOpNode(Kind op, AST_PTR tree1, AST_PTR tree2)
{
  char *exp_string = (char *)safe_allocate(strlen(tree1->exp_string) + 
				           strlen(tree2->exp_string) + 6);
  char *lp1="", *rp1="", *lp2="", *rp2="";
  char *op_string;
  AST_PTR tree_tmp;

  switch (op) {
  case Alt: op_string = "^"; break;
  case Diff: op_string = "-"; break;
  case And: op_string = "&"; break;
  case Or: op_string = "|"; break;
  default: op_string = "";
  }

  if (op == Seq || op == Alt) {
    if (tree1->op == Epsilon) return tree2;
    if (tree1->op == Empty) return tree1;
    if (tree2->op == Epsilon) return tree1;
    if (tree2->op == Empty) return tree2;
  }
  
  if (op == And) {
    if (tree1->op == Epsilon) return tree1;
    if (tree1->op == Empty) return tree1;
    if (tree2->op == Epsilon) return tree2;
    if (tree2->op == Empty) return tree2;
  }
  if (op == Diff) {
    if (tree1->op == Empty) return tree1;
    if (tree2->op == Empty) return tree1;
  }

  if (tree1 == tree2){
    if (op == Or || op == And) return tree1;
    if (op == Diff) return mkEmpty();
  }
  
  if (op == Or) 
    sprintf(exp_string,"%s%s%s", tree1->exp_string,
	    op_string, tree2->exp_string);
  else {
    if (op == Diff && tree2->op == Diff) {
      lp2 ="("; rp2 = ")"; 
    } else {
    if (tree1->op < op) { 
      lp1 ="("; rp1=")";
    }
    if (tree2->op < op) {
      lp2 ="("; rp2 = ")";
    }
    }
    sprintf(exp_string,"%s%s%s%s%s%s%s", lp1, tree1->exp_string, rp1,
	    op_string, lp2, tree2->exp_string, rp2);
  }
  tree_tmp = lookup (exp_string);
  
  if (tree_tmp != NULL)  {
    free(exp_string);
    return tree_tmp;
  }

  tree_tmp = (AST_PTR ) safe_allocate(sizeof *tree_tmp);
  tree_tmp->hash = hash(exp_string);
  tree_tmp->op = op;
  tree_tmp->exp_string = exp_string;
  tree_tmp->nullable = (op == Or?tree1->nullable || tree2->nullable:
			(op == Diff? tree1->nullable*(tree1->nullable - tree2->nullable)
			 : tree1->nullable && tree2->nullable));
  tree_tmp->lf = NULL;
  tree_tmp->state = -1;
  tree_tmp->lchild = tree1;
  tree_tmp->rchild = tree2;
  return insert(tree_tmp); 
}

AST_PTR insert_op_node(Kind op, AST_PTR tree1, AST_PTR tree2)
{
  
  AST_PTR e1 = tree1->lchild, e2 = tree1->rchild;
  /* (e1 || e2) || e1 = (e1 || e2) || e2 = e1 || e2 */ 

  if (tree1->op != op) {
    /* if (tree1->hash == tree2->hash) { */
    /*   printf("hash equal\n"); */
    /* } */
    if (tree1->hash <= tree2->hash)
      return mkOpNode(op, tree1, tree2);
    else 
      return mkOpNode(op, tree2, tree1);
  }
  

  if (e2->hash <= tree2->hash) 
    return mkOpNode(op, tree1, tree2);

  if (e1->op != op) {
    if (tree2->hash <= e1->hash)
      return mkOpNode(op, mkOpNode(op, tree2, e1), e2);
    else return mkOpNode(op, mkOpNode(op, e1, tree2), e2);
  }

  return mkOpNode(op, insert_op_node(op, e1, tree2), e2); 
 
}



AST_PTR arrangeSeqNode(AST_PTR L, AST_PTR C)
{
  /* TODO */
  // 输入"(a|b)c"，化简后输出"ac|bc"；输入"a(b|c)", 输出"ab|ac"

  if (L->op == Or) {
		return arrangeOpNode(Or, arrangeSeqNode(L->lchild, C), arrangeSeqNode(L->rchild, C));
	} else if (C->op == Or){
	
		return  arrangeOpNode(Or, arrangeSeqNode(L, C->lchild), arrangeSeqNode(L, C->rchild));
	} else {
	   return  mkOpNode(Seq, L, C);
	}
}




AST_PTR mkStarNode(AST_PTR tree)
{
  char *exp_string = (char *) safe_allocate(strlen(tree->exp_string) + 4);
  char *lp = "", *rp = "";
  AST_PTR tree_tmp;

  if (tree->op == Star || tree->op == Epsilon ||
      tree->op == Empty) return tree;

  if (tree->op == Or && tree->lchild->op == Epsilon) return mkStarNode(tree->rchild);
    
  if (tree->op != Alpha) {
    lp = "("; rp = ")";
  }

  sprintf(exp_string, "%s%s%s%c", lp, tree->exp_string, rp, '*');

  tree_tmp = lookup (exp_string);
  
  if (tree_tmp != NULL)  {
    free(exp_string);
    return tree_tmp;
  }
 
  tree_tmp = (AST_PTR) safe_allocate(sizeof(*tree_tmp));
  
  tree_tmp->hash = hash(exp_string);
  tree_tmp->op = Star;
  tree_tmp->exp_string = exp_string;
  tree_tmp->nullable = 1;
  tree_tmp->state = -1;
  tree_tmp->lf = NULL;
  tree_tmp->lchild = tree;
  tree_tmp->rchild = NULL;
  return insert(tree_tmp); 
}

AST_PTR arrangeOpNode(Kind op, AST_PTR tree1, AST_PTR tree2)
{
  /* TODO */
//   实现交换律化简正则表达式函数 AST_PTR arrangeOpNode(Kind op, AST_PTR tree1, AST_PTR
// tree2), 其中 op 为 |, ^ 或 &. 要求对连续的 | 均转换为最左结合的 | 运算.
// 实现函数 AST_PTR arrangeSeqNode(AST_PTR tree1, AST_PTR tree2), 用结合率化简正则表达式. 如
// 输入"(a|b)c"，化简后输出"ac|bc"；输入"a(b|c)", 输出"ab|ac".

  AST_PTR left;

  if (op == Or || op == Alt || op == And) {

		if (op == tree1->op && op == tree2->op) {

			left = arrangeOpNode(op, tree1, tree2->rchild);
			return arrangeOpNode(op, left, tree2->lchild);

		} else if (op == tree1->op) {
	
			if (tree1->rchild->hash > tree2->hash) {

				if (tree1->lchild->op == op) {

					left = arrangeOpNode(op, tree1->lchild, tree2);
					return mkOpNode(op, left, tree1->rchild);

				} else {
					if (tree2->hash > tree1->lchild->hash) {

						left = mkOpNode(op, tree1->lchild, tree2);

						return mkOpNode(op, left, tree1->rchild);
					} else {

						left = mkOpNode(op, tree2, tree1->lchild);

						return mkOpNode(op, left, tree1->rchild);
					}
				}
			} else {
				return mkOpNode(op, tree1, tree2);
        
			}
		} else if (op == tree2->op) {
			return arrangeOpNode(op, tree2, tree1);
		} else {
			return insert_op_node(op, tree1, tree2);
		}
	} else {
		return insert_op_node(op, tree1, tree2);
	}









}

static FILE *gv_file;
static int label_count = 0;

int graphviz_ast_aux(AST_PTR tree )
{
  int count = label_count ++, left_count,
    right_count;
  char op_c;
  
  switch (tree -> op ) {
  case Alpha: case Empty: case Epsilon:
    fprintf(gv_file, "node_%d[label=\"%s\"]\n", count, tree->exp_string);
    return count;
  case Star: 
    left_count = graphviz_ast_aux( tree -> lchild);
    fprintf(gv_file, "node_%d[label=\"%c\", shape=circle]\n", count, '*');
    fprintf(gv_file, "node_%d  -> node_%d[dir=none];\n", 
            count, left_count);
    return count;
  case Seq:
    op_c = '.'; break;
  case Or:
    op_c = '|'; break;
  case Diff:
    op_c = '-'; break;
  case Alt:
    op_c = '^'; break;
  case And:
    op_c = '&';
  }

  left_count = graphviz_ast_aux(tree->lchild);
  right_count = graphviz_ast_aux(tree->rchild);
  
  fprintf(gv_file, "node_%d[label=\"%c\", shape=circle]\n", count, op_c);
  fprintf(gv_file, "node_%d  -> node_%d[dir=none];\n", count, left_count);
  fprintf(gv_file, "node_%d  -> node_%d[dir=none];\n", count, right_count);

  return count;
}

void graphviz_ast( AST_PTR tree )
{
  if ((gv_file = fopen("ast.gv", "w")) == NULL) {
   printf("coudn't create output file!\n");
   exit(1);
  }

  if (tree == NULL) {
    printf("attempt output empty tree!\n");
    exit (1);
  }
  
  fprintf(gv_file, "digraph  G {label =\"%s\";\n", tree->exp_string); 
  graphviz_ast_aux ( tree );
  fprintf(gv_file, "}\n");
  fclose (gv_file);
}

