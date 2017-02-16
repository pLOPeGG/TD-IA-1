/* PageRank */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>



/* allocate one object of given type */
#define	NEW(type)	((type*)calloc((size_t)1,(size_t)sizeof(type)))

/* allocate num objects of given type */
#define	NEW_A(num,type)	((type*)calloc((size_t)(num),(size_t)sizeof(type)))

typedef	unsigned int u_int;
typedef double Real;

/* vector definition */
typedef	struct
{
  u_int	dim;
  Real *ve;
} VEC;

/* matrix definition */
typedef	struct
{
  u_int	m, n;
  Real **me;
} MAT;

/* sparse matrix definition */
typedef struct graphnode
{
  u_int col;
  Real val;
  struct graphnode *next;
} NODE;

typedef struct
{
  u_int m, n;
  NODE *rows;
} SMAT;

/* v_get -- gets a VEC of dimension 'dim'
   Precondition: size >= 0
   Postcondition: initialized to zero */
VEC	*v_get(u_int size)
{
  VEC *v;
  
  if ( (v = NEW(VEC)) == (VEC *)NULL )
  {
    fprintf(stderr, "v_get memory error");
    exit(-1);
  }
  
  v->dim = size;
  if ( (v->ve = NEW_A(size,Real)) == (Real *)NULL )
  {
    free(v);
    fprintf(stderr, "v_get memory error");
    exit(-1);
  }
  
  return (v);
}

/* v_free -- returns VEC & associated memory back to memory heap */
int	v_free(VEC *vec)
{
  if ( vec == (VEC *)NULL )
    return (-1);
  
  if ( vec->ve == (Real *)NULL ) 
  {
    free(vec);
  }
  else
  {
    free(vec->ve);
    free(vec);
  }
  
  return (0);
}

SMAT *sm_get(u_int m, u_int n)
{
  SMAT *G;

  if ( (G = NEW(SMAT)) == (SMAT *)NULL )
  {
    fprintf(stderr, "sm_get memory error");
    exit(-1);
  }
  
  G->m = m ; G->n = n;

  if ((G->rows = NEW_A(m,NODE)) == (NODE *)NULL )
  {	
    free(G);
    fprintf(stderr, "sm_get memory error");
    exit(-1);
  }

  for (u_int i=0 ; i<G->m ; i++)
    (G->rows[i]).val = -1;

  return (G);
}

int	sm_free(SMAT *G)
{
  if ( G == (SMAT *)NULL )
    return (-1);
  
  if ( G->rows == (NODE *)NULL ) 
  {
    free(G);
  }
  else
  {
    NODE *n0;
    NODE *n1;
    for (u_int i=0 ; i<G->m ; i++)
    {
      n0 = &(G->rows[i]);
      if (n0->val < 0.0) break; /* empty line */
      n0 = n0->next;
      while (n0->val >= 0.0)
      {
        n1 = n0->next;
        free(n0);
        n0 = n1;
      }
      free(n0);
    }
    free(G->rows);
    free(G);
  }
  
  return (0);
}

NODE *sm_add(NODE *n0, u_int c, Real v)
{
  NODE *n1;
  n0->col = c;
  n0->val = v;
  if ( (n1 = NEW(NODE)) == (NODE *)NULL )
  {
    fprintf(stderr, "sm_add memory error");
    exit(-1);
  }
  n1->val = -1;
  n0->next = n1;
  return (n1);
}

/* m_get -- gets an mxn matrix by dynamic memory allocation 
   Precondition: m>=0 && n>=0
   Postcondition: initialized to zero */
MAT	*m_get(u_int m, u_int n)
{
  MAT	*g;
  
  if ( (g = NEW(MAT)) == (MAT *)NULL )
  {
    fprintf(stderr, "m_get memory error");
    exit(-1);
  }
  
  g->m = m ; g->n = n;

  if ((g->me = NEW_A(m,Real*)) == (Real **)NULL )
  {	
    free(g);
    fprintf(stderr, "m_get memory error");
    exit(-1);
  }
  
  for ( int i = 0; i < m; i++ )
    if ( (g->me[i] = NEW_A(n,Real)) == (Real *)NULL )
    {
      fprintf(stderr, "m_get memory error");
      exit(-1);
    }
  
  return (g);
}

/* m_free -- returns MAT & associated memory back to memory heap */
int	m_free(MAT *mat)
{
  if ( mat == (MAT *)NULL )
    return (-1);
  
  for ( int i = 0; i < mat->m; i++ )
    if ( mat->me[i] != (Real *)NULL ) free(mat->me[i]);

  if ( mat->me != (Real **)NULL ) free(mat->me);
  
  free(mat);
  
  return (0);
}

/* m_input -- file input of matrix */
MAT *m_input(FILE *fp)
{
  MAT *g;
  u_int m,n,val;
  
  /* get dimension */
  if ( fscanf(fp," Matrix: %u by %u",&m,&n) < 2 )
  {
    fprintf(stderr, "m_input error reading dimensions");
    exit(-1);
  }
  
  /* allocate memory if necessary */
  g = m_get(m,n);
  
  /* get entries */
  for ( u_int i = 0 ; i < m; i++ )
  {
	  if ( fscanf(fp," row %u:",&val) < 1 )
    {
      fprintf(stderr, "m_input error reading line %u", i);
      exit(-1);
    }
	  for ( u_int j = 0; j < n; j++ )
	    if ( fscanf(fp,"%lf",&g->me[i][j]) < 1 )
      {
        fprintf(stderr, "m_input error reading line %u col %u", i, j);
        exit(-1);
      }
  }
  
  return (g);
}

/* sm_input -- file input of sparse matrix */
SMAT *sm_input(FILE *fp)
{
  SMAT *g;
  u_int m,n,row;
  Real col;
  NODE *n0;
  
  /* get dimension */
  if ( fscanf(fp," SparseMatrix: %u by %u",&m,&n) < 2 )
  {
    fprintf(stderr, "sm_input error reading dimensions");
    exit(-1);
  }
  
  g = sm_get(m,n);
  
  /* get entries */
  for ( u_int i = 0 ; i < m; i++ )
  {
	  if ( fscanf(fp," row %u:",&row) < 1 )
    {
      fprintf(stderr, "sm_input error reading line %u", i);
      exit(-1);
    }
    n0 = &(g->rows[i]);
	  for (;;)
    {
	    if ( fscanf(fp,"%lf",&col) < 1 )
      {
        fprintf(stderr, "sm_input error reading line %u col x", i);
        exit(-1);
      }
      if (col < 0.0) break;
      n0 = sm_add(n0, (u_int)col, 1.0);
    }
  }
  
  return (g);
}

static char *format = "%1.5g ";

void sm_output(FILE *fp, SMAT *G)
{
  NODE *n0;

  fprintf(fp, "SparseMatrix: %d by %d\n", G->m, G->n);
  for ( u_int i = 0 ; i < G->m ; i++ )
  {
    fprintf(fp, "row %u: ", i); 
    n0 = &(G->rows[i]);
    while (n0->val >= 0.0)
    {
      fprintf(fp,format,(Real)n0->col);
      n0 = n0->next;
    }
    fprintf(fp,"-1\n");
  }
}

/* m_output -- file output of matrix 
   Precondition: Memory already allocated for the matrix */
void m_output(FILE *fp, MAT *g)
{
   u_int tmp;
   
   fprintf(fp, "Matrix: %d by %d\n", g->m, g->n);
   for ( u_int i = 0 ; i < g->m ; i++ )
   {
     fprintf(fp,"row %u: ", i);
     for ( u_int j = 0, tmp = 2 ; j < g->n ; j++, tmp++ )
     {
       fprintf(fp,format,g->me[i][j]);
       if ( ! (tmp % 9) ) putc('\n',fp);
     }
     if ( tmp % 9 != 1 ) putc('\n',fp);
   }
}

/* v_output -- file output of vector */
void v_output(FILE *fp, VEC *v)
{
  fprintf(fp, "Vector: %d\n", v->dim);
  for (u_int i=0 ; i<v->dim ; i++) fprintf(fp,format,v->ve[i]);
  putc('\n',fp);
}

/* m_cp -- copy matrix M in OUT
   Precondition: memory is already allocated for M and OUT
   Precondition: sizes of M and OUT must match*/
MAT *m_cp(MAT *M, MAT *OUT)
{
  for ( u_int i = 0; i < M->m; i++ )
    memmove(&(OUT->me[i][0]), &(M->me[i][0]), (M->n)*sizeof(Real));

  return (OUT);
}

/* v_cp -- copy vector v in out
   Precondition: memory is already allocated for v and out*/
VEC *v_cp(VEC *v, VEC *out)
{
  memmove(&(out->ve[0]), &(v->ve[0]), (v->dim)*sizeof(Real));
  return (out);
}

VEC * r0_get(u_int n)
{
  VEC *vector = v_get(n);
  u_int i;
  for(i=0;i<n;i++)
    {
      vector->ve[i]=1./n;
    }
  return vector;
 }

VEC * v_mult_m(VEC * vect, MAT * matr)
{
  VEC * result = v_get(vect->dim);
  u_int i,j;
  Real tmp;
  for(i=0;i<result->dim;i++)
    {
     tmp = 0;
     for(j=0;j<result->dim;j++)
       {
	 tmp+=vect->ve[j]*matr->me[j][i];
       }
     result->ve[i]=tmp;
    }
  
  return result;
}

VEC * v_mult_m_puiss(VEC *v,MAT *m,u_int p)
{
  VEC *result=v_get(v->dim);
  VEC *tmp;
  v_cp(v,result);
  u_int i;
  for(i=0;i<p;i++)
    {
      
      result=v_mult_m(tmp=result,m);
      //if(i){v_free(tmp);}
    }
  return result;
}

MAT * h_get(MAT *M)
{
  MAT *result = m_get(M->m, M->n);
  u_int i,j,cmpt;
  for(i=0;i<result->m;i++)
    {
      cmpt=0;
      for(j=0;j<result->n;j++)
	{
	  if(M->me[i][j]){cmpt++;}
	}
      for(j=0;j<result->n;j++)
	{
	  if(M->me[i][j]){result->me[i][j]=1./cmpt;}
	}
    }
  return result;
}

MAT * s_get(MAT *M)
{
MAT *result = m_get(M->m, M->n);
  u_int i,j,cmpt;
  for(i=0;i<result->m;i++)
    {
      cmpt=0;
      for(j=0;j<result->n;j++)
	{
	  if(M->me[i][j]){cmpt++;}
	}
      for(j=0;j<result->n;j++)
	{
	  if(cmpt==0){result->me[i][j]=1./result->n;}
	  else if(M->me[i][j]){result->me[i][j]=1./cmpt;}
	}
    }
  return result;
}

MAT * e_get(MAT * S, Real alpha)
{
  MAT * e=m_get(S->m,S->n);
  m_cp(S,e);
  u_int i,j;
    for(i=0;i<e->m;i++)
    {
      for(j=0;j<e->n;j++)
	{
	  e->me[i][j]*=alpha;
	  e->me[i][j]+=(1-alpha)/e->n;  
	}
    }
    return e;
}

VEC* v_mult_real(VEC * v, Real a)
{
  u_int i;
  for(i=0;i<v->dim;i++)
    {
      v->ve[i]*=a;
    }
  return v;
}

void h_get_sm(SMAT * sm)
{
  SMAT * result = sm_get(sm->m,sm->n);
  u_int i,cmpt;
  NODE * no; 
  for(i=0;i<result->m;i++)
    {
      no=&(sm->rows[i]);
      cmpt=0;
      while(no->val!=-1)
	{
	  cmpt++;
	  no=no->next;
	}
      no=&(sm->rows[i]);
      while(no->val!=-1)
	{
	  no->val=1./cmpt;
	  no=no->next;
	}
    }
  
}
VEC * a_get(SMAT * sm)
{
  VEC * result =v_get(sm->m);
  NODE * no;
  u_int i;
  for(i=0;i<result->dim;i++)
    {
      no=&(sm->rows[i]);
      if(no->val==-1)
	{
	  result->ve[i]=1;
	}
      
    }
  return result;
}

Real prod_scal(VEC * v1, VEC *v2)
{
  u_int i;
  Real result=0;
  for(i=0;i<v1->dim;i++)
    {
      result+=v1->ve[i]*v2->ve[i];
    }
  return result;
}

VEC * v_mult_sm(VEC * v, SMAT * sm)
{
  VEC * result=v_get(v->dim);
  u_int i,j;
  NODE *node;
  for(i=0;i<v->dim;i++)
    {
      node=&(sm->rows[i]);
      while(node->val!=-1)
	{
	  result->ve[node->col]+=v->ve[i]*node->val;
	  node=node->next;
	}
    }
  return result;
}

VEC * iter(VEC * v_prec, SMAT * sm, Real alpha, VEC * a)
{
  VEC *result = v_get(v_prec->dim);
  v_cp(v_mult_real(v_mult_sm(v_prec,sm),alpha),result);
  Real offset = (alpha * prod_scal(v_prec,a)+1-alpha)/result->dim;
  u_int i;
  for(i=0;i<result->dim;i++)
    {
      result->ve[i]+=offset;
    }
  return result;
}

VEC * many_iter(VEC * v_prec, SMAT * sm, Real alpha, u_int p)
{
  VEC *result=v_get(v_prec->dim);
  VEC *tmp;
  VEC *a=a_get(sm);
  v_cp(v_prec,result);
  u_int i;
  for(i=0;i<p;i++)
    {
      
      result=iter(tmp=result,sm,alpha,a);
     
      //if(i){v_free(tmp);}
    }
  return result;
}

#define NB_PAGE_CONTROL 1000

void boost(SMAT * sm, u_int page)
{
  u_int i;
  NODE * no;
  for(i=0;i< NB_PAGE_CONTROL ;i++)
    {
      if(page==i){continue;}
      no=&(sm->rows[i]);
      while(no->val!=-1)
	{
	  if(no->col==page){break;}
	  no=no->next;
	}
      if(no->col!=page)
	{
	  no=sm_add(no,page,1);
	}
    }
}


      
void main()
{
  MAT *G;

  FILE *fp;
  fp = fopen("dataset/g.dat", "r");
  G = m_input(fp);
  fclose(fp);


  VEC *R=r0_get(G->n);
  MAT *H=h_get(G);
  MAT *S=s_get(G);
  MAT *E=e_get(S,0.9);
  VEC *R1=v_mult_m_puiss(R,E,2);
  v_output(stdout,R1);
  m_free(G);
  v_free(R);
  
  SMAT *SG;
  fp = fopen("dataset/genetic.dat","r");
  SG = sm_input(fp);
  fclose(fp);
  boost(SG,267);
  h_get_sm(SG);
  R=r0_get(SG->m);
  VEC *R2=many_iter(R,SG,0.9,20000);
  //v_output(stdout,R2);

  printf("\nCase 267 : %f\n",R2->ve[267]);

  
  fp = fopen("dataset/test.dat","w");
  sm_output(fp,SG);
  sm_free(SG);

  exit(0);
}
