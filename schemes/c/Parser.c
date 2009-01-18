/*---- license ----*/
/*-------------------------------------------------------------------------
 Coco.ATG -- Attributed Grammar
 Compiler Generator Coco/R,
 Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
 extended by M. Loeberbauer & A. Woess, Univ. of Linz
 with improvements by Pat Terry, Rhodes University.
 ported to C by Charles Wang <charlesw123456@gmail.com>

 This program is free software; you can redistribute it and/or modify it 
 under the terms of the GNU General Public License as published by the 
 Free Software Foundation; either version 2, or (at your option) any 
 later version.

 This program is distributed in the hope that it will be useful, but 
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 for more details.

 You should have received a copy of the GNU General Public License along 
 with this program; if not, write to the Free Software Foundation, Inc., 
 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 As an exception, it is allowed to write an extension of Coco/R that is
 used as a plugin in non-free software.

 If not otherwise stated, any source code generated by Coco/R (other than 
 Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
/*---- enable ----*/
#include  "Parser.h"
#include  "c/Token.h"

/*---- cIncludes ----*/
#include  <ctype.h>
#include  "Globals.h"
#include  "lexical/CharSet.h"
#include  "lexical/CharClass.h"
#include  "lexical/Nodes.h"
#include  "syntax/Nodes.h"
static const int CcsParser_id = 0;
static const int CcsParser_str = 1;
static const char * noString = "~none~";
/*---- enable ----*/

static void CcsParser_SynErr(CcsParser_t * self, int n);
static const char * set[];

static void
CcsParser_Get(CcsParser_t * self)
{
    if (self->t) CcsScanner_TokenDecRef(&self->scanner, self->t);
    self->t = self->la;
    for (;;) {
	self->la = CcsScanner_Scan(&self->scanner);
	if (self->la->kind <= self->maxT) { /*++self->errDist;*/ break; }
	/* May be implement pragmas here is wrong... But I still not found any
	 * needs to use pragmas, so just leave it along. */
	/*---- Pragmas ----*/
	if (self->la->kind == 47) {
	    
	}
	/*---- enable ----*/
    }
}

static CcsBool_t
CcsParser_StartOf(CcsParser_t * self, int s)
{
    return set[s][self->la->kind] == '*';
}

static void
CcsParser_Expect(CcsParser_t * self, int n)
{
    if (self->la->kind == n) CcsParser_Get(self);
    else CcsParser_SynErr(self, n);
}

#ifdef CcsParser_WEAK_USED
static void
CcsParser_ExpectWeak(CcsParser_t * self, int n, int follow)
{
    if (self->la->kind == n) CcsParser_Get(self);
    else {
	CcsParser_SynErr(self, n);
	while (!CcsParser_StartOf(self, follow)) CcsParser_Get(self);
    }
}

static CcsBool_t
CcsParser_WeakSeparator(CcsParser_t * self, int n, int syFol, int repFol)
{
    if (self->la->kind == n) { CcsParser_Get(self); return TRUE; }
    else if (CcsParser_StartOf(self, repFol)) { return FALSE; }
    CcsParser_SynErr(self, n);
    while (!(CcsParser_StartOf(self, syFol) ||
	     CcsParser_StartOf(self, repFol) ||
	     CcsParser_StartOf(self, 0)))
	CcsParser_Get(self);
    return CcsParser_StartOf(self, syFol);
}
#endif /* CcsParser_WEAK_USED */

/*---- ProductionsHeader ----*/
static void CcsParser_Coco(CcsParser_t * self);
static void CcsParser_SchemeDecl(CcsParser_t * self);
static void CcsParser_SectionDecl(CcsParser_t * self);
static void CcsParser_SetDecl(CcsParser_t * self);
static void CcsParser_TokenDecl(CcsParser_t * self, const CcObjectType_t * typ);
static void CcsParser_TokenExpr(CcsParser_t * self, CcGraph_t ** g);
static void CcsParser_Set(CcsParser_t * self, CcCharSet_t ** s);
static void CcsParser_AttrDecl(CcsParser_t * self, CcSymbolNT_t * sym);
static void CcsParser_SemText(CcsParser_t * self, CcsPosition_t ** pos);
static void CcsParser_Expression(CcsParser_t * self, CcGraph_t ** g);
static void CcsParser_SimSet(CcsParser_t * self, CcCharSet_t ** s);
static void CcsParser_Char(CcsParser_t * self, int * n);
static void CcsParser_Sym(CcsParser_t * self, char ** name, int * kind);
static void CcsParser_Term(CcsParser_t * self, CcGraph_t ** g);
static void CcsParser_Resolver(CcsParser_t * self, CcsPosition_t ** pos);
static void CcsParser_Factor(CcsParser_t * self, CcGraph_t ** g);
static void CcsParser_Attribs(CcsParser_t * self, CcNode_t * p);
static void CcsParser_Condition(CcsParser_t * self);
static void CcsParser_TokenTerm(CcsParser_t * self, CcGraph_t ** g);
static void CcsParser_TokenFactor(CcsParser_t * self, CcGraph_t ** g);
/*---- enable ----*/

void
CcsParser_Parse(CcsParser_t * self)
{
    self->t = NULL;
    self->la = CcsScanner_GetDummy(&self->scanner);
    CcsParser_Get(self);
    /*---- ParseRoot ----*/
    CcsParser_Coco(self);
    /*---- enable ----*/
    CcsParser_Expect(self, 0);
}

void
CcsParser_SemErr(CcsParser_t * self, const CcsToken_t * token,
		 const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, &token->loc, format, ap);
    va_end(ap);
}

void
CcsParser_SemErrT(CcsParser_t * self, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, &self->t->loc, format, ap);
    va_end(ap);
}

static CcsBool_t
CcsParser_Init(CcsParser_t * self)
{
    self->t = self->la = NULL;
    /*---- constructor ----*/
    self->maxT = 46;
    if (!CcGlobals(&self->globals, &self->errpool)) return FALSE;
    self->tokenString = NULL;
    self->genScanner = FALSE;
    self->symtab = &self->globals.symtab;
    self->lexical = self->globals.lexical;
    self->syntax = &self->globals.syntax;
    /*---- enable ----*/
    return TRUE;
}

CcsParser_t *
CcsParser(CcsParser_t * self, FILE  * infp, FILE * errfp)
{
    if (!CcsErrorPool(&self->errpool, errfp)) goto errquit0;
    if (!CcsScanner(&self->scanner, &self->errpool, infp)) goto errquit1;
    if (!CcsParser_Init(self)) goto errquit2;
    return self;
 errquit2:
    CcsScanner_Destruct(&self->scanner);
 errquit1:
    CcsErrorPool_Destruct(&self->errpool);
 errquit0:
    return NULL;
}

CcsParser_t *
CcsParser_ByName(CcsParser_t * self, const char * infn, FILE * errfp)
{
    if (!CcsErrorPool(&self->errpool, errfp)) goto errquit0;
    if (!CcsScanner_ByName(&self->scanner, &self->errpool, infn))
	goto errquit1;
    if (!CcsParser_Init(self)) goto errquit2;
    return self;
 errquit2:
    CcsScanner_Destruct(&self->scanner);
 errquit1:
    CcsErrorPool_Destruct(&self->errpool);
 errquit0:
    return NULL;
}

void
CcsParser_Destruct(CcsParser_t * self)
{
    /*---- destructor ----*/
    if (self->tokenString && self->tokenString != noString)
	CcFree(self->tokenString);
    CcGlobals_Destruct(&self->globals);
    /*---- enable ----*/
    if (self->la) CcsScanner_TokenDecRef(&self->scanner, self->la);
    if (self->t) CcsScanner_TokenDecRef(&self->scanner, self->t);
    CcsScanner_Destruct(&self->scanner);
    CcsErrorPool_Destruct(&self->errpool);
}

/*---- ProductionsBody ----*/
static void
CcsParser_Coco(CcsParser_t * self)
{
    CcSymbol_t  * sym;
    CcGraph_t   * g, * g1, * g2;
    char        * gramName = NULL;
    CcCharSet_t * s; 
    CcsToken_t  * beg;
    CcsBool_t     nested;
    CcsBool_t     undef;
    CcsBool_t     noAttrs;
    self->tokenString = NULL; 
    while (self->la->kind == 23 || self->la->kind == 24) {
	if (self->la->kind == 23) {
	    CcsParser_SchemeDecl(self);
	} else {
	    CcsParser_SectionDecl(self);
	}
    }
    CcsParser_Expect(self, 6);
    self->genScanner = TRUE; 
    CcsParser_Expect(self, 1);
    gramName = CcStrdup(self->t->val); 
    if (self->la->kind == 7) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 1)) {
	    CcsParser_Get(self);
	}
	self->syntax->members = CcsScanner_GetPosition(&self->scanner, beg, self->la);
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    }
    if (self->la->kind == 8) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 2)) {
	    CcsParser_Get(self);
	}
	self->syntax->constructor = CcsScanner_GetPosition(&self->scanner, beg, self->la);
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    }
    if (self->la->kind == 9) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 3)) {
	    CcsParser_Get(self);
	}
	self->syntax->destructor = CcsScanner_GetPosition(&self->scanner, beg, self->la);
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    }
    if (self->la->kind == 10) {
	CcsParser_Get(self);
	while (self->la->kind == 3) {
	    CcsParser_Get(self);
	    CcLexical_SetOption(self->lexical, self->t); 
	}
    }
    if (self->la->kind == 11) {
	CcsParser_Get(self);
	while (self->la->kind == 1) {
	    CcsParser_SetDecl(self);
	}
    }
    if (self->la->kind == 12) {
	CcsParser_Get(self);
	while (self->la->kind == 1 || self->la->kind == 3 || self->la->kind == 5) {
	    CcsParser_TokenDecl(self, symbol_t);
	}
    }
    if (self->la->kind == 13) {
	CcsParser_Get(self);
	while (self->la->kind == 1 || self->la->kind == 3 || self->la->kind == 5) {
	    CcsParser_TokenDecl(self, symbol_pr);
	}
    }
    while (self->la->kind == 14) {
	CcsParser_Get(self);
	nested = FALSE; 
	CcsParser_Expect(self, 15);
	CcsParser_TokenExpr(self, &g1);
	CcsParser_Expect(self, 16);
	CcsParser_TokenExpr(self, &g2);
	if (self->la->kind == 17) {
	    CcsParser_Get(self);
	    nested = TRUE; 
	}
	CcLexical_NewComment(self->lexical, self->t,
			     g1->head, g2->head, nested);
	CcGraph_Destruct(g1); CcGraph_Destruct(g2); 
    }
    while (self->la->kind == 18) {
	CcsParser_Get(self);
	CcsParser_Set(self, &s);
	CcCharSet_Or(self->lexical->ignored, s);
	CcCharSet_Destruct(s); 
    }
    while (!(self->la->kind == 0 || self->la->kind == 19)) {
	CcsParser_SynErr(self, 47); CcsParser_Get(self);
    }
    CcsParser_Expect(self, 19);
    if (self->genScanner) CcLexical_MakeDeterministic(self->lexical);
    CcEBNF_Clear(&self->lexical->base);
    CcEBNF_Clear(&self->syntax->base); 
    while (self->la->kind == 1) {
	CcsParser_Get(self);
	sym = CcSymbolTable_FindSym(self->symtab, self->t->val);
	undef = (sym == NULL);
	if (undef) {
	    sym = CcSymbolTable_NewNonTerminal(self->symtab,
					       self->t->val, self->t->loc.line);
	} else {
	    if (sym->base.type == symbol_nt) {
		if (((CcSymbolNT_t *)sym)->graph != NULL)
		    CcsParser_SemErrT(self, "name declared twice");
	    } else {
		CcsParser_SemErrT(self, "this symbol kind not allowed on left side of production");
	    }
	    sym->line = self->t->loc.line;
	}
	CcsAssert(sym->base.type == symbol_nt);
	noAttrs = (((CcSymbolNT_t *)sym)->attrPos == NULL);
	if (!noAttrs) {
	    CcsPosition_Destruct(((CcSymbolNT_t *)sym)->attrPos);
	    ((CcSymbolNT_t *)sym)->attrPos = NULL; 
	} 
	if (self->la->kind == 29 || self->la->kind == 31) {
	    CcsParser_AttrDecl(self, (CcSymbolNT_t *)sym);
	}
	if (!undef && noAttrs != (((CcSymbolNT_t *)sym)->attrPos == NULL))
	    CcsParser_SemErrT(self, "attribute mismatch between declaration and use of this symbol"); 
	if (self->la->kind == 44) {
	    CcsParser_SemText(self, &((CcSymbolNT_t *)sym)->semPos);
	}
	CcsParser_ExpectWeak(self, 20, 4);
	CcsParser_Expression(self, &g);
	((CcSymbolNT_t *)sym)->graph = g->head;
	CcGraph_Finish(g);
	CcGraph_Destruct(g); 
	CcsParser_ExpectWeak(self, 21, 5);
    }
    CcsParser_Expect(self, 22);
    CcsParser_Expect(self, 1);
    if (strcmp(gramName, self->t->val))
	CcsParser_SemErrT(self, "name does not match grammar name");
    self->syntax->gramSy = CcSymbolTable_FindSym(self->symtab, gramName);
    CcFree(gramName);
    if (self->syntax->gramSy == NULL) {
	CcsParser_SemErrT(self, "missing production for grammar name");
    } else {
	sym = self->syntax->gramSy;
	if (((CcSymbolNT_t *)sym)->attrPos != NULL)
	    CcsParser_SemErrT(self, "grammar symbol must not have attributes");
    }
    /* noSym gets highest number */
    self->syntax->noSy = CcSymbolTable_NewTerminal(self->symtab, "???", 0);
    CcSyntax_SetupAnys(self->syntax); 
    CcsParser_Expect(self, 21);
}

static void
CcsParser_SchemeDecl(CcsParser_t * self)
{
    CcsParser_Expect(self, 23);
    CcsParser_Expect(self, 1);
    if (self->syntax->schemeName)
	CcFree(self->syntax->schemeName);
    self->syntax->schemeName = CcStrdup(self->t->val); 
    CcsParser_Expect(self, 1);
    if (self->syntax->grammarPrefix)
	CcFree(self->syntax->grammarPrefix);
    self->syntax->grammarPrefix = CcStrdup(self->t->val); 
}

static void
CcsParser_SectionDecl(CcsParser_t * self)
{
    char * secname; CcsToken_t * beg; 
    CcsParser_Expect(self, 24);
    CcsParser_Expect(self, 1);
    secname = CcStrdup(self->t->val);
    CcsScanner_TokenIncRef(&self->scanner, beg = self->t); 
    while (CcsParser_StartOf(self, 6)) {
	CcsParser_Get(self);
    }
    CcGlobals_NewSection(&self->globals, secname,
			 CcsScanner_GetPositionBetween(&self->scanner, beg, self->la));
    CcsScanner_TokenDecRef(&self->scanner, beg);
    CcFree(secname); 
    CcsParser_Expect(self, 22);
    CcsParser_Expect(self, 21);
}

static void
CcsParser_SetDecl(CcsParser_t * self)
{
    CcCharSet_t * s; CcsToken_t * nameToken;
    const char * name; CcCharClass_t * c; 
    CcsParser_Expect(self, 1);
    name = self->t->val;
    CcsScanner_TokenIncRef(&self->scanner, nameToken = self->t);
    c = CcLexical_FindCharClassN(self->lexical, name);
    if (c != NULL)
	CcsParser_SemErrT(self, "name '%s' declared twice", name); 
    CcsParser_Expect(self, 20);
    CcsParser_Set(self, &s);
    if (CcCharSet_Elements(s) == 0)
	CcsParser_SemErrT(self, "character set must not be empty");
    CcLexical_NewCharClass(self->lexical, name, s);
    CcsScanner_TokenDecRef(&self->scanner, nameToken); 
    CcsParser_Expect(self, 21);
}

static void
CcsParser_TokenDecl(CcsParser_t * self, const CcObjectType_t * typ)
{
    char * name = NULL; int kind; CcSymbol_t * sym; CcGraph_t * g; 
    CcsParser_Sym(self, &name, &kind);
    sym = CcSymbolTable_FindSym(self->symtab, name);
    if (sym != NULL) {
	CcsParser_SemErrT(self, "name '%s' declared twice", name);
    } else if (typ == symbol_t) {
	sym = CcSymbolTable_NewTerminal(self->symtab, name, self->t->loc.line);
	((CcSymbolT_t *)sym)->tokenKind = symbol_fixedToken;
    } else if (typ == symbol_pr) {
	sym = CcSymbolTable_NewPragma(self->symtab, name, self->t->loc.line);
	((CcSymbolPR_t *)sym)->tokenKind = symbol_fixedToken;
	((CcSymbolPR_t *)sym)->semPos = NULL;
    }
    if (self->tokenString && self->tokenString != noString)
	CcFree(self->tokenString);
    self->tokenString = NULL;
    CcFree(name); 
    while (!(CcsParser_StartOf(self, 0))) {
	CcsParser_SynErr(self, 48); CcsParser_Get(self);
    }
    if (self->la->kind == 20) {
	CcsParser_Get(self);
	CcsParser_TokenExpr(self, &g);
	CcsParser_Expect(self, 21);
	if (kind == CcsParser_str)
	    CcsParser_SemErrT(self, "a literal must not be declared with a structure");
	CcGraph_Finish(g);
	if (self->tokenString == NULL || self->tokenString == noString) {
	    CcLexical_ConvertToStates(self->lexical, g->head, sym);
	} else { /* CcsParser_TokenExpr is a single string */
	    if (CcHashTable_Get(&self->lexical->literals,
				self->tokenString) != NULL)
		CcsParser_SemErrT(self, "token string '%s' declared twice", self->tokenString);
	    CcHashTable_Set(&self->lexical->literals,
			    self->tokenString, (CcObject_t *)sym);
	    CcLexical_MatchLiteral(self->lexical, self->t,
				   self->tokenString, sym);
	    CcFree(self->tokenString);
	}
	self->tokenString = NULL;
	CcGraph_Destruct(g); 
    } else if (CcsParser_StartOf(self, 7)) {
	if (kind == CcsParser_id) self->genScanner = FALSE;
	else CcLexical_MatchLiteral(self->lexical, self->t, sym->name, sym); 
    } else CcsParser_SynErr(self, 49);
    if (self->la->kind == 44) {
	CcsParser_SemText(self, &((CcSymbolPR_t *)sym)->semPos);
	if (typ != symbol_pr)
	    CcsParser_SemErrT(self, "semantic action not allowed here"); 
    }
}

static void
CcsParser_TokenExpr(CcsParser_t * self, CcGraph_t ** g)
{
    CcGraph_t * g2; CcsBool_t first; 
    CcsParser_TokenTerm(self, g);
    first = TRUE; 
    while (CcsParser_WeakSeparator(self, 33, 9, 8)) {
	CcsParser_TokenTerm(self, &g2);
	if (first) { CcEBNF_MakeFirstAlt(&self->lexical->base, *g); first = FALSE; }
	CcEBNF_MakeAlternative(&self->lexical->base, *g, g2);
	CcGraph_Destruct(g2); 
    }
}

static void
CcsParser_Set(CcsParser_t * self, CcCharSet_t ** s)
{
    CcCharSet_t * s2; 
    CcsParser_SimSet(self, s);
    while (self->la->kind == 25 || self->la->kind == 26) {
	if (self->la->kind == 25) {
	    CcsParser_Get(self);
	    CcsParser_SimSet(self, &s2);
	    CcCharSet_Or(*s, s2);
	    CcCharSet_Destruct(s2); 
	} else {
	    CcsParser_Get(self);
	    CcsParser_SimSet(self, &s2);
	    CcCharSet_Subtract(*s, s2);
	    CcCharSet_Destruct(s2); 
	}
    }
}

static void
CcsParser_AttrDecl(CcsParser_t * self, CcSymbolNT_t * sym)
{
    CcsToken_t * beg; 
    if (self->la->kind == 29) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 10)) {
	    if (CcsParser_StartOf(self, 11)) {
		CcsParser_Get(self);
	    } else {
		CcsParser_Get(self);
		CcsParser_SemErrT(self, "bad string in attributes"); 
	    }
	}
	CcsParser_Expect(self, 30);
	sym->attrPos = CcsScanner_GetPosition(&self->scanner, beg, self->t);
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    } else if (self->la->kind == 31) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 12)) {
	    if (CcsParser_StartOf(self, 13)) {
		CcsParser_Get(self);
	    } else {
		CcsParser_Get(self);
		CcsParser_SemErrT(self, "bad string in attributes"); 
	    }
	}
	CcsParser_Expect(self, 32);
	sym->attrPos = CcsScanner_GetPosition(&self->scanner, beg, self->t);
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    } else CcsParser_SynErr(self, 50);
}

static void
CcsParser_SemText(CcsParser_t * self, CcsPosition_t ** pos)
{
    CcsToken_t * beg; 
    CcsParser_Expect(self, 44);
    CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
    while (CcsParser_StartOf(self, 14)) {
	if (CcsParser_StartOf(self, 15)) {
	    CcsParser_Get(self);
	} else if (self->la->kind == 4) {
	    CcsParser_Get(self);
	    CcsParser_SemErrT(self, "bad string in semantic action"); 
	} else {
	    CcsParser_Get(self);
	    CcsParser_SemErrT(self, "missing end of previous semantic action"); 
	}
    }
    CcsParser_Expect(self, 45);
    *pos = CcsScanner_GetPosition(&self->scanner, beg, self->t);
    CcsScanner_TokenDecRef(&self->scanner, beg); 
}

static void
CcsParser_Expression(CcsParser_t * self, CcGraph_t ** g)
{
    CcGraph_t * g2; CcsBool_t first; 
    CcsParser_Term(self, g);
    first = TRUE; 
    while (CcsParser_WeakSeparator(self, 33, 17, 16)) {
	CcsParser_Term(self, &g2);
	if (first) { CcEBNF_MakeFirstAlt(&self->syntax->base, *g); first = FALSE; }
	CcEBNF_MakeAlternative(&self->syntax->base, *g, g2);
	CcGraph_Destruct(g2); 
    }
}

static void
CcsParser_SimSet(CcsParser_t * self, CcCharSet_t ** s)
{
    int n1, n2;
    CcCharClass_t * c;
    const char * cur0;
    int ch;
    char * cur, * name;
    int idx; 
    *s = CcCharSet(); 
    if (self->la->kind == 1) {
	CcsParser_Get(self);
	c = CcLexical_FindCharClassN(self->lexical, self->t->val);
	if (c != NULL) CcCharSet_Or(*s, c->set);
	else CcsParser_SemErrT(self, "undefined name '%s'", self->t->val); 
    } else if (self->la->kind == 3) {
	CcsParser_Get(self);
	name = CcUnescape(self->t->val);
	if (self->lexical->ignoreCase) {
	    for (cur = name; *cur; ++cur) *cur = tolower(*cur);
	}
	cur0 = name;
	while (*cur0) {
	    ch = CcsUTF8GetCh(&cur0, name + strlen(name));
	    CcsAssert(ch >= 0);
	    CcCharSet_Set(*s, ch);
	}
	CcFree(name); 
    } else if (self->la->kind == 5) {
	CcsParser_Char(self, &n1);
	CcCharSet_Set(*s, n1); 
	if (self->la->kind == 27) {
	    CcsParser_Get(self);
	    CcsParser_Char(self, &n2);
	    for (idx = n1; idx <= n2; ++idx) CcCharSet_Set(*s, idx); 
	}
    } else if (self->la->kind == 28) {
	CcsParser_Get(self);
	CcCharSet_Fill(*s, COCO_WCHAR_MAX); 
    } else CcsParser_SynErr(self, 51);
}

static void
CcsParser_Char(CcsParser_t * self, int * n)
{
    char * name; const char * cur; 
    CcsParser_Expect(self, 5);
    *n = 0;
    cur = name = CcUnescape(self->t->val);
    *n = CcsUTF8GetCh(&cur, name + strlen(name));
    if (*cur != 0)
	CcsParser_SemErrT(self, "unacceptable character value: '%s'", self->t->val);
    CcFree(name);
    if (self->lexical->ignoreCase) *n = tolower(*n); 
}

static void
CcsParser_Sym(CcsParser_t * self, char ** name, int * kind)
{
    char * cur; 
    *name = NULL; 
    if (self->la->kind == 1) {
	CcsParser_Get(self);
	*kind = CcsParser_id; *name = CcStrdup(self->t->val); 
    } else if (self->la->kind == 3 || self->la->kind == 5) {
	if (self->la->kind == 3) {
	    CcsParser_Get(self);
	    *name = CcUnescape(self->t->val); 
	} else {
	    CcsParser_Get(self);
	    *name = CcUnescape(self->t->val); 
	}
	*kind = CcsParser_str;
	if (self->lexical->ignoreCase) {
	    for (cur = *name; *cur; ++cur) *cur = tolower(*cur);
	}
	if (strchr(*name, ' '))
	    CcsParser_SemErrT(self, "literal tokens \"%s\" can not contain blanks", *name); 
    } else CcsParser_SynErr(self, 52);
    if (!*name) *name = CcStrdup("???"); 
}

static void
CcsParser_Term(CcsParser_t * self, CcGraph_t ** g)
{
    CcGraph_t * g2; CcsPosition_t * pos; CcNode_t * rslv = NULL;
    *g = NULL; 
    if (CcsParser_StartOf(self, 18)) {
	if (self->la->kind == 42) {
	    CcsParser_Resolver(self, &pos);
	    rslv = CcEBNF_NewNode(&self->syntax->base,
				  CcNodeRslvP(self->la->loc.line, pos));
	    *g = CcGraphP(rslv); 
	}
	CcsParser_Factor(self, &g2);
	if (rslv == NULL) *g = g2;
	else {
	    CcEBNF_MakeSequence(&self->syntax->base, *g, g2);
	    CcGraph_Destruct(g2);
	} 
	while (CcsParser_StartOf(self, 19)) {
	    CcsParser_Factor(self, &g2);
	    CcEBNF_MakeSequence(&self->syntax->base, *g, g2);
	    CcGraph_Destruct(g2); 
	}
    } else if (CcsParser_StartOf(self, 20)) {
	*g = CcGraphP(CcEBNF_NewNode(&self->syntax->base, CcNodeEps(0))); 
    } else CcsParser_SynErr(self, 53);
    if (*g == NULL) /* invalid start of Term */
	*g = CcGraphP(CcEBNF_NewNode(&self->syntax->base, CcNodeEps(0))); 
}

static void
CcsParser_Resolver(CcsParser_t * self, CcsPosition_t ** pos)
{
    CcsToken_t * beg; 
    CcsParser_Expect(self, 42);
    CcsParser_Expect(self, 35);
    CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
    CcsParser_Condition(self);
    *pos = CcsScanner_GetPosition(&self->scanner, beg, self->t);
    CcsScanner_TokenDecRef(&self->scanner, beg); 
}

static void
CcsParser_Factor(CcsParser_t * self, CcGraph_t ** g)
{
    char * name = NULL;
    int kind;
    CcsPosition_t * pos;
    CcsBool_t weak = FALSE; 
    CcSymbol_t * sym;
    CcsBool_t undef;
    CcNode_t * p;
    *g = NULL; 
    switch (self->la->kind) {
    case 1: case 3: case 5: case 34: {
	if (self->la->kind == 34) {
	    CcsParser_Get(self);
	    weak = TRUE; 
	}
	CcsParser_Sym(self, &name, &kind);
	sym = CcSymbolTable_FindSym(self->symtab, name);
	if (sym == NULL && kind == CcsParser_str)
	    sym = (CcSymbol_t *)CcHashTable_Get(&self->lexical->literals, name);
	undef = (sym == NULL);
	if (undef) {
	    if (kind == CcsParser_id) {
		/* forward nt */
		sym = CcSymbolTable_NewNonTerminal(self->symtab, name, 0);
	    } else if (self->genScanner) {
		sym = CcSymbolTable_NewTerminal(self->symtab, name, self->t->loc.line);
		CcLexical_MatchLiteral(self->lexical, self->t, sym->name, sym);
	    } else {  /* undefined string in production */
		CcsParser_SemErrT(self, "undefined string in production");
		sym = self->syntax->eofSy;  /* dummy */
	    }
	} else {
	    if (kind == CcsParser_str) {
		if (sym->base.type != symbol_t) {
		    CcsParser_SemErrT(self, "%s should be terminal, but it is not.", name);
		} else if (((CcSymbolT_t *)sym)->tokenKind != symbol_litToken &&
			   ((CcSymbolT_t *)sym)->tokenKind != symbol_fixedToken) {
		    CcsParser_SemErrT(self, "%s should be literal, but it is not.", name);
		}
	    }
	}
	CcFree(name);
	if (sym->base.type != symbol_t && sym->base.type != symbol_nt)
	    CcsParser_SemErrT(self, "this symbol kind is not allowed in a production");
	if (weak) {
	    if (sym->base.type != symbol_t)
		CcsParser_SemErrT(self, "only terminals may be weak");
	}
	p = CcSyntax_NodeFromSymbol(self->syntax, sym, self->t->loc.line, weak);
	*g = CcGraphP(p); 
	if (self->la->kind == 29 || self->la->kind == 31) {
	    CcsParser_Attribs(self, p);
	    if (kind != CcsParser_id)
		CcsParser_SemErrT(self, "a literal must not have attributes"); 
	}
	if (undef) {
	    if (sym->base.type == symbol_nt)
		((CcSymbolNT_t *)sym)->attrPos = CcsPosition_Clone(((CcNodeNT_t *)p)->pos);
	} else if (sym->base.type == symbol_nt &&
		   (((CcNodeNT_t *)p)->pos == NULL) !=
		   (((CcSymbolNT_t *)sym)->attrPos == NULL))
	    CcsParser_SemErrT(self, "attribute mismatch between declaration and use of this symbol"); 
	break;
    }
    case 35: {
	CcsParser_Get(self);
	CcsParser_Expression(self, g);
	CcsParser_Expect(self, 36);
	break;
    }
    case 37: {
	CcsParser_Get(self);
	CcsParser_Expression(self, g);
	CcsParser_Expect(self, 38);
	CcEBNF_MakeOption(&self->syntax->base, *g); 
	break;
    }
    case 39: {
	CcsParser_Get(self);
	CcsParser_Expression(self, g);
	CcsParser_Expect(self, 40);
	CcEBNF_MakeIteration(&self->syntax->base, *g); 
	break;
    }
    case 44: {
	CcsParser_SemText(self, &pos);
	p = CcEBNF_NewNode(&self->syntax->base, CcNodeSem(0));
	((CcNodeSEM_t *)p)->pos = pos;
	*g = CcGraphP(p); 
	break;
    }
    case 28: {
	CcsParser_Get(self);
	p = CcEBNF_NewNode(&self->syntax->base, CcNodeAny(0));
	*g = CcGraphP(p); 
	break;
    }
    case 41: {
	CcsParser_Get(self);
	p = CcEBNF_NewNode(&self->syntax->base, CcNodeSync(0));
	*g = CcGraphP(p);
	break;
    }
    default: CcsParser_SynErr(self, 54); break;
    }
    if (*g == NULL) /* invalid start of Factor */
	*g = CcGraphP(CcEBNF_NewNode(&self->syntax->base, CcNodeEps(0))); 
}

static void
CcsParser_Attribs(CcsParser_t * self, CcNode_t * p)
{
    CcsToken_t * beg; 
    if (self->la->kind == 29) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 10)) {
	    if (CcsParser_StartOf(self, 11)) {
		CcsParser_Get(self);
	    } else {
		CcsParser_Get(self);
		CcsParser_SemErrT(self, "bad string in attributes"); 
	    }
	}
	CcsParser_Expect(self, 30);
	CcNode_SetPosition(p, CcsScanner_GetPosition(&self->scanner,
						     beg, self->t));
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    } else if (self->la->kind == 31) {
	CcsParser_Get(self);
	CcsScanner_TokenIncRef(&self->scanner, beg = self->la); 
	while (CcsParser_StartOf(self, 12)) {
	    if (CcsParser_StartOf(self, 13)) {
		CcsParser_Get(self);
	    } else {
		CcsParser_Get(self);
		CcsParser_SemErrT(self, "bad string in attributes"); 
	    }
	}
	CcsParser_Expect(self, 32);
	CcNode_SetPosition(p, CcsScanner_GetPosition(&self->scanner,
						     beg, self->t));
	CcsScanner_TokenDecRef(&self->scanner, beg); 
    } else CcsParser_SynErr(self, 55);
}

static void
CcsParser_Condition(CcsParser_t * self)
{
    while (CcsParser_StartOf(self, 21)) {
	if (self->la->kind == 35) {
	    CcsParser_Get(self);
	    CcsParser_Condition(self);
	} else {
	    CcsParser_Get(self);
	}
    }
    CcsParser_Expect(self, 36);
}

static void
CcsParser_TokenTerm(CcsParser_t * self, CcGraph_t ** g)
{
    CcGraph_t * g2; 
    CcsParser_TokenFactor(self, g);
    while (CcsParser_StartOf(self, 9)) {
	CcsParser_TokenFactor(self, &g2);
	CcEBNF_MakeSequence(&self->lexical->base, *g, g2);
	CcGraph_Destruct(g2); 
    }
    if (self->la->kind == 43) {
	CcsParser_Get(self);
	CcsParser_Expect(self, 35);
	CcsParser_TokenExpr(self, &g2);
	CcLexical_SetContextTrans(self->lexical, g2->head);
	self->lexical->hasCtxMoves = TRUE;
	CcEBNF_MakeSequence(&self->lexical->base, *g, g2); 
	CcsParser_Expect(self, 36);
    }
}

static void
CcsParser_TokenFactor(CcsParser_t * self, CcGraph_t ** g)
{
    char * name = NULL; int kind;
    CcTransition_t trans;
    CcCharClass_t * c; 
    *g = NULL; 
    if (self->la->kind == 1 || self->la->kind == 3 || self->la->kind == 5) {
	CcsParser_Sym(self, &name, &kind);
	if (kind == CcsParser_id) {
	    c = CcLexical_FindCharClassN(self->lexical, name);
	    if (c == NULL) {
		CcsParser_SemErrT(self, "undefined name");
		c = CcLexical_NewCharClass(self->lexical, name, CcCharSet());
	    }
	    CcTransition_FromCharSet(&trans, c->set, trans_normal,
				     &self->lexical->classes);
	    *g = CcGraphP(CcEBNF_NewNode(&self->lexical->base,
					 CcNodeTrans(0, &trans)));
	    CcTransition_Destruct(&trans);
	    if (self->tokenString && self->tokenString != noString)
		CcFree(self->tokenString);
	    self->tokenString = (char *)noString;
	} else { /* CcsParser_str */
	    *g = CcLexical_StrToGraph(self->lexical, name, self->t);
	    if (self->tokenString == NULL) self->tokenString = CcStrdup(name);
	    else {
		if (self->tokenString != noString) CcFree(self->tokenString);
		self->tokenString = (char *)noString;
	    }
	}
	CcFree(name); 
    } else if (self->la->kind == 35) {
	CcsParser_Get(self);
	CcsParser_TokenExpr(self, g);
	CcsParser_Expect(self, 36);
    } else if (self->la->kind == 37) {
	CcsParser_Get(self);
	CcsParser_TokenExpr(self, g);
	CcsParser_Expect(self, 38);
	CcEBNF_MakeOption(&self->lexical->base, *g); 
    } else if (self->la->kind == 39) {
	CcsParser_Get(self);
	CcsParser_TokenExpr(self, g);
	CcsParser_Expect(self, 40);
	CcEBNF_MakeIteration(&self->lexical->base, *g); 
    } else CcsParser_SynErr(self, 56);
    if (*g == NULL) /* invalid start of TokenFactor */
      *g = CcGraphP(CcEBNF_NewNode(&self->lexical->base, CcNodeEps(0))); 
}

/*---- enable ----*/

static void
CcsParser_SynErr(CcsParser_t * self, int n)
{
    const char * s; char format[20];
    switch (n) {
    /*---- SynErrors ----*/
    case 0: s = "\"" "EOF" "\" expected"; break;
    case 1: s = "\"" "ident" "\" expected"; break;
    case 2: s = "\"" "number" "\" expected"; break;
    case 3: s = "\"" "string" "\" expected"; break;
    case 4: s = "\"" "badString" "\" expected"; break;
    case 5: s = "\"" "char" "\" expected"; break;
    case 6: s = "\"" "COMPILER" "\" expected"; break;
    case 7: s = "\"" "MEMBERS" "\" expected"; break;
    case 8: s = "\"" "CONSTRUCTOR" "\" expected"; break;
    case 9: s = "\"" "DESTRUCTOR" "\" expected"; break;
    case 10: s = "\"" "OPTIONS" "\" expected"; break;
    case 11: s = "\"" "CHARACTERS" "\" expected"; break;
    case 12: s = "\"" "TOKENS" "\" expected"; break;
    case 13: s = "\"" "PRAGMAS" "\" expected"; break;
    case 14: s = "\"" "COMMENTS" "\" expected"; break;
    case 15: s = "\"" "FROM" "\" expected"; break;
    case 16: s = "\"" "TO" "\" expected"; break;
    case 17: s = "\"" "NESTED" "\" expected"; break;
    case 18: s = "\"" "IGNORE" "\" expected"; break;
    case 19: s = "\"" "PRODUCTIONS" "\" expected"; break;
    case 20: s = "\"" "=" "\" expected"; break;
    case 21: s = "\"" "." "\" expected"; break;
    case 22: s = "\"" "END" "\" expected"; break;
    case 23: s = "\"" "SCHEME" "\" expected"; break;
    case 24: s = "\"" "SECTION" "\" expected"; break;
    case 25: s = "\"" "+" "\" expected"; break;
    case 26: s = "\"" "-" "\" expected"; break;
    case 27: s = "\"" ".." "\" expected"; break;
    case 28: s = "\"" "ANY" "\" expected"; break;
    case 29: s = "\"" "<" "\" expected"; break;
    case 30: s = "\"" ">" "\" expected"; break;
    case 31: s = "\"" "<." "\" expected"; break;
    case 32: s = "\"" ".>" "\" expected"; break;
    case 33: s = "\"" "|" "\" expected"; break;
    case 34: s = "\"" "WEAK" "\" expected"; break;
    case 35: s = "\"" "(" "\" expected"; break;
    case 36: s = "\"" ")" "\" expected"; break;
    case 37: s = "\"" "[" "\" expected"; break;
    case 38: s = "\"" "]" "\" expected"; break;
    case 39: s = "\"" "{" "\" expected"; break;
    case 40: s = "\"" "}" "\" expected"; break;
    case 41: s = "\"" "SYNC" "\" expected"; break;
    case 42: s = "\"" "IF" "\" expected"; break;
    case 43: s = "\"" "CONTEXT" "\" expected"; break;
    case 44: s = "\"" "(." "\" expected"; break;
    case 45: s = "\"" ".)" "\" expected"; break;
    case 46: s = "\"" "???" "\" expected"; break;
    case 47: s = "invalid \"" "Coco" "\""; break;
    case 48: s = "invalid \"" "TokenDecl" "\""; break;
    case 49: s = "this symbol not expected in \"" "TokenDecl" "\""; break;
    case 50: s = "this symbol not expected in \"" "AttrDecl" "\""; break;
    case 51: s = "this symbol not expected in \"" "SimSet" "\""; break;
    case 52: s = "this symbol not expected in \"" "Sym" "\""; break;
    case 53: s = "this symbol not expected in \"" "Term" "\""; break;
    case 54: s = "this symbol not expected in \"" "Factor" "\""; break;
    case 55: s = "this symbol not expected in \"" "Attribs" "\""; break;
    case 56: s = "this symbol not expected in \"" "TokenFactor" "\""; break;
    /*---- enable ----*/
    default:
	snprintf(format, sizeof(format), "error %d", n);
	s = format;
	break;
    }
    CcsParser_SemErr(self, self->la, "%s", s);
}

static const char * set[] = {
    /*---- InitSet ----*/
    /*    5    0    5    0    5    0    5    0    5  */
    "**.*.*.......**...***.......................*...", /* 0 */
    ".*******.......***..***************************.", /* 1 */
    ".********......***..***************************.", /* 2 */
    ".*********.....***..***************************.", /* 3 */
    "**.*.*.......**...****......*....***.*.*.**.*...", /* 4 */
    "**.*.*.......**...***.*.....................*...", /* 5 */
    ".*********************.************************.", /* 6 */
    ".*.*.*.......**...**........................*...", /* 7 */
    "..............*.****.*..............*.*.*.......", /* 8 */
    ".*.*.*.............................*.*.*........", /* 9 */
    ".*****************************.****************.", /* 10 */
    ".***.*************************.****************.", /* 11 */
    ".*******************************.**************.", /* 12 */
    ".***.***************************.**************.", /* 13 */
    ".********************************************.*.", /* 14 */
    ".***.***************************************..*.", /* 15 */
    ".....................*..............*.*.*.......", /* 16 */
    ".*.*.*...............*......*....**********.*...", /* 17 */
    ".*.*.*......................*.....**.*.*.**.*...", /* 18 */
    ".*.*.*......................*.....**.*.*.*..*...", /* 19 */
    ".....................*...........*..*.*.*.......", /* 20 */
    ".***********************************.**********."  /* 21 */
    /*---- enable ----*/
};
