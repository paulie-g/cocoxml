/*-------------------------------------------------------------------------
  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>

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
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <ctype.h>
#include  <errno.h>
#include  <libgen.h>
#include  <limits.h>
#include  <unistd.h>
#include  "config.h"
#include  "OutputScheme.h"
#include  "Arguments.h"
#include  "BasicObjects.h"

static const char * _8tab_ = "\t\t\t\t\t\t\t\t";
static const char * _7space_ = "       ";

static void
WriteSpaces(FILE * outfp, int spaces)
{
    while (spaces >= 64) {
	fprintf(outfp, "%s", _8tab_);
	spaces -= 64;
    }
    if (spaces >= 8) {
	fprintf(outfp, "%s", _8tab_ + (8 - spaces / 8));
	spaces %= 8;
    }
    if (spaces > 0)
	fprintf(outfp, "%s", _7space_ + (7 - spaces));
}

void
CcPrintf(CcOutput_t * self, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(self->outfp, format, ap); 
    va_end(ap);
}
void
CcPrintfI(CcOutput_t * self, const char * format, ...)
{
    va_list ap;
    WriteSpaces(self->outfp, self->indent);
    va_start(ap, format);
    vfprintf(self->outfp, format, ap); 
    va_end(ap);
}

static const char *
GetEOL(const char * start, char * eol)
{
    const char * end = start + strcspn(start, "\r\n");
    if (*end == 0) return end;
    *eol++ = end[0]; *eol = 0;
    if (end[0] == end[1] || (end[1] != '\r' && end[1] != '\n')) return end + 1;
    *eol++ = end[1]; *eol = 0;
    return end + 2;
}

void
CcSource(CcOutput_t * self, const CcsPosition_t * pos)
{
    char eol[3]; int curcol;
    const char * start, * cur, * end;
    CcsBool_t isBlank, hasEOL;

    eol[0] = 0;
    start = pos->text; end = GetEOL(start, eol);
    WriteSpaces(self->outfp, self->indent);
    fwrite(start, end - start, 1, self->outfp);
    isBlank = FALSE;
    hasEOL = pos->text < end && (end[-1] == '\r' || end[-1] == '\n');

    while (*end) {
	start = end; end = GetEOL(start, eol);
	curcol = 0;
	for (cur = start; *cur == ' ' || *cur == '\t'; ++cur)
	    curcol += (*cur == ' ' ? 1 : 8 - curcol % 8);
	isBlank = *cur == 0 || *cur == '\r' || *cur == '\n';
	if (!isBlank) {
	    /* Only non-blank line is outputed */
	    if (curcol <= pos->col) WriteSpaces(self->outfp, self->indent);
	    else WriteSpaces(self->outfp, self->indent + (curcol - pos->col));
	    fwrite(cur, end - cur, 1, self->outfp);
	    hasEOL = start < end && (end[-1] == '\r' || end[-1] == '\n');
	}
    }
    if (!isBlank && !hasEOL) {
	/* There is no EOL in the last line, add it */
	if (eol[0]) fwrite(eol, strlen(eol), 1, self->outfp);
	else fputc('\n', self->outfp);
    }
}

CcOutputScheme_t *
CcOutputScheme(const CcOutputSchemeType_t * type, CcGlobals_t * globals,
	       CcArguments_t * arguments)
{
    CcOutputScheme_t * self = (CcOutputScheme_t *)CcObject(&type->base);
    self->globals = globals;
    self->arguments = arguments;
    return (CcOutputScheme_t *)self;
}

void
CcOutputScheme_Destruct(CcObject_t * self)
{
    CcObject_Destruct(self);
}

typedef struct {
    const char * extension;
    const char * start;
    const char * end;
}  CommentMark_t;

static const CommentMark_t cmarr[] = {
    { ".html", "<!---- ", " ---->" },
    { NULL, "/*---- ", " ----*/" }
};

static const CommentMark_t *
Path2CommentMark(const char * tempPath)
{
    int l0 = strlen(tempPath), l1; const CommentMark_t * curcm;
    for (curcm = cmarr; curcm->extension; ++curcm) {
	l1 = strlen(curcm->extension);
	if (l0 > l1 && !strcmp(tempPath + (l0 - l1), curcm->extension))
	    return curcm;
    }
    return curcm;
}

static CcsBool_t
LocateMark(const char ** b, const char ** e,
	   const char * lmark, const char * rmark)
{
    int llen = strlen(lmark), rlen = strlen(rmark);
    int tlen = llen + rlen;
    while (*b < *e && isspace(**b)) ++*b;
    while (*b < *e && isspace(*(*e - 1))) --*e;
    if (*e - *b < tlen) return FALSE;
    if (strncmp(*b, lmark, llen)) return FALSE;
    if (strncmp(*e - rlen, rmark, rlen)) return FALSE;
    *b += llen; *e -= rlen;
    while (*b < *e && isspace(**b)) ++*b;
    while (*b < *e && isspace(*(*e - 1))) --*e;
    return TRUE;
}

static void
GetResult(char * dest, size_t destlen, const char * src, size_t srclen)
{
    if (srclen < destlen - 1) {
	memcpy(dest, src, srclen); dest[srclen] = 0;
    } else {
	memcpy(dest, src, destlen - 1); dest[destlen - 1] = 0;
    }
}

static CcsBool_t
CheckMark(const char * lnbuf, const char * startMark, const char * endMark,
	  int * retIndent, char * retCommand, size_t szRetCommand,
	  char * retParamStr, size_t szRetParamStr)
{
    const char * b, * e, * start;
    if (!*lnbuf) return FALSE;
    b = lnbuf; e = lnbuf + strlen(lnbuf);

    *retIndent = 0;
    for (b = lnbuf; *b == ' ' || *b == '\t'; ++b)
	*retIndent += (*b == ' ' ? 1 : 8);

    if (!LocateMark(&b, &e, startMark, endMark)) return FALSE;
    start = b;
    while (b < e && isalnum(*b)) ++b;
    GetResult(retCommand, szRetCommand, start, b - start);

    while (b < e && isspace(*b)) ++b;
    if (*b != '(') *retParamStr = 0;
    else {
	start = b + 1;
	while (b <= e && *b != ')') ++b;
	GetResult(retParamStr, szRetParamStr, start, b - start);
	++b;
    }
    return TRUE;
}

static const char * const suffixes[] = {
    "Parser", "Scanner", NULL
};

static void
CcOutputScheme_Write(CcOutputScheme_t * self, char * lnbuf, FILE * outfp)
{
    size_t srcprefixlen;
    char * start0, * start1, * cur;
    const char * const * suffix;
    const char * srcprefix = self->globals->templatePrefix;
    const char * tgtprefix = self->globals->syntax.grammarPrefix;

    start0 = lnbuf;
    if (tgtprefix) {
	srcprefixlen = strlen(srcprefix);
	start1 = lnbuf;
	for (;;) {
	    cur = strstr(start1, srcprefix);
	    if (cur == NULL) break;
	    for (suffix = suffixes; *suffix; ++suffix)
		if (!strncmp(cur + srcprefixlen, *suffix, strlen(*suffix))) {
		    *cur = 0; fputs(start0, outfp);
		    fputs(tgtprefix, outfp);
		    fputs(*suffix, outfp);
		    start0 = start1 = cur + srcprefixlen + strlen(*suffix);
		    break;
		}
	    if (!*suffix) start1 = cur + srcprefixlen;
	}
    }
    fputs(start0, outfp);
}

static CcsBool_t
CcOutputScheme_ApplyTemplate(CcOutputScheme_t * self,
			     const char * srcPath, const char * tgtPath)
{
    FILE * srcfp, * tgtfp;
    char tgtOutPath[PATH_MAX];
    char lnbuf[4096]; CcsBool_t enabled;
    char Command[128], ParamStr[128], replacedPrefix[128];
    CcOutput_t output;
    const CcsPosition_t * pos;
    const CommentMark_t * srcCM = Path2CommentMark(srcPath);
    const CcOutputSchemeType_t * type =
	(const CcOutputSchemeType_t *)self->base.type;

    if (!(srcfp = fopen(srcPath, "r"))) {
	fprintf(stderr, "open %s for read failed.\n", srcPath);
	goto errquit0;
    }
    if (!strcmp(srcPath, tgtPath)) {
	snprintf(tgtOutPath, sizeof(tgtOutPath), "%s.out", tgtPath);
	if (!(tgtfp = fopen(tgtOutPath, "w"))) {
	    fprintf(stderr, "open %s for write failed.\n", tgtOutPath);
	    goto errquit1;
	}
    } else {
	tgtOutPath[0] = 0;
	if (!(tgtfp = fopen(tgtPath, "w"))) {
	    fprintf(stderr, "open %s for write failed.\n", tgtPath);
	    goto errquit1;
	}
    }

    printf("Updating %s to %s.....\n", srcPath, tgtPath);
    output.outfp = tgtfp;
    enabled = TRUE; replacedPrefix[0] = 0;
    while (fgets(lnbuf, sizeof(lnbuf), srcfp)) {
	if (!CheckMark(lnbuf, srcCM->start, srcCM->end, &output.indent,
		       Command, sizeof(Command), ParamStr, sizeof(ParamStr))) {
	    /* Common line */
	    if (enabled) CcOutputScheme_Write(self, lnbuf, tgtfp);
	    continue;
	}
	fputs(lnbuf, tgtfp);
	if (!strcmp(Command, "enable")) {
	    enabled = TRUE;
	} else if ((pos = CcGlobals_GetSection(self->globals, Command))) {
	    CcPrintf(&output, "%s", pos->text);
	    enabled = FALSE;
	} else {
	    if (!type->write(self, &output, Command, ParamStr))
		goto errquit2;
	    enabled = FALSE;
	}
    }

    fclose(tgtfp);
    fclose(srcfp);

    if (tgtOutPath[0]) {
	remove(tgtPath);
	if (rename(tgtOutPath, tgtPath) < 0) {
	    fprintf(stderr, "Rename from '%s' to '%s' failed: %s\n",
		    tgtOutPath, tgtPath, strerror(errno));
	    return FALSE;
	}
    }
    return TRUE;
 errquit2:
    fclose(tgtfp);
    if (tgtOutPath[0]) remove(tgtOutPath);
 errquit1:
    fclose(srcfp);
 errquit0:
    return FALSE;
}

typedef struct {
    const char * dir;
    const char * dir1;
    const char * scheme;
}  CcPathInfo_t;

static void
PathJoin(char * result, size_t szresult, const char * dir, const char * dir1,
	 const char * scheme, const char * filename)
{
    if (scheme == NULL) {
	snprintf(result, szresult, "%s/%s", dir, filename);
    } else if (dir1 == NULL) {
	snprintf(result, szresult, "%s/%s/%s", dir, scheme, filename);
    } else {
	snprintf(result, szresult, "%s/%s/%s/%s", dir, dir1, scheme, filename);
    }
}

static CcsBool_t
CheckFile(const char * filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0) return FALSE;
    return S_ISREG(st.st_mode);
}

CcsBool_t
CcOutputScheme_GenerateOutputs(CcOutputScheme_t * self,
			       const char * schemeName, const char * atgname)
{
    CcArgumentsIter_t iter; CcArrayListIter_t iter0;
    const char * output, * outdir, * tempdir;
    CcPathInfo_t pathes[5], * curpath;
    const CcString_t * update;
    char srcPath[PATH_MAX], tgtPath[PATH_MAX];
    char * atgname0 = NULL, * selfpath0 = NULL;
    const CcArrayList_t * updates = &self->globals->updates;

    output = CcArguments_First(self->arguments, "output", &iter);
    if (!output)  output = "auto";
    outdir = CcArguments_First(self->arguments, "dir", &iter);
    if (!outdir) outdir = dirname(atgname0 = CcStrdup(atgname));
    tempdir = CcArguments_First(self->arguments, "tempdir", &iter);

    /* Put all template directories into pathes. */
    memset(pathes, 0, sizeof(pathes));
    curpath = pathes;
    if (strcmp(output, "generate")) { /* Not generate only. */
	curpath->dir = outdir; ++curpath;
    }
    if (strcmp(output, "update")) { /* Not update only. */
	if (tempdir) { curpath->dir = tempdir; ++curpath; }
	curpath->dir = dirname(selfpath0 =
			       CcStrdup(self->arguments->selfpath));
	curpath->dir1 = "schemes";
	curpath->scheme = schemeName;
	++curpath;
	curpath->dir = DATADIR;
	curpath->dir1 = PACKAGE;
	curpath->scheme = schemeName;
	++curpath;
    }
    CcsAssert(curpath - pathes < sizeof(pathes) / sizeof(pathes[0]));
    curpath->dir = NULL;

    /* For all template files */
    for (update = (const CcString_t *)CcArrayList_FirstC(updates, &iter0);
	 update;
	 update = (const CcString_t *)CcArrayList_NextC(updates, &iter0)) {
	for (curpath = pathes; curpath->dir; ++curpath) {
	    PathJoin(srcPath, sizeof(srcPath), curpath->dir, curpath->dir1,
		     curpath->scheme, update->value);
	    if (CheckFile(srcPath)) break;
	}
	if (!curpath->dir) {
	    fprintf(stderr,
		    "Can not find any template file for %s.\n", update->value);
	    break;
	}
	PathJoin(tgtPath, sizeof(tgtPath), outdir, NULL, NULL, update->value);
	if (!CcOutputScheme_ApplyTemplate(self, srcPath, tgtPath)) break;
    }

    if (atgname0) CcFree(atgname0);
    if (selfpath0) CcFree(selfpath0);
    return update == NULL;
}
