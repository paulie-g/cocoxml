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
#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "Frame.h"

static Bool_t
LocateMark(const char ** b, const char ** e,
	   const char * lmark, const char * rmark)
{
    int llen = strlen(lmark), rlen = strlen(rmark);
    int tlen = llen + rlen;
    while (*b <= *e && isspace(**b)) ++*b;
    while (*b <= *e && isspace(**e)) --*e;
    if (*e - *b < tlen) return FALSE;
    if (strncmp(*b, lmark, llen)) return FALSE;
    if (strncmp(*e - rlen, rmark, rlen)) return FALSE;
    *b += llen; *e -= rlen;
    while (*b <= *e && isspace(**b)) ++*b;
    while (*b <= *e && isspace(**e)) --*e;
    return TRUE;
}

static void
GetResult(char * dest, size_t destlen, const char * src, size_t srclen)
{
    if (srclen < destlen) strncpy(dest, src, srclen);
    else strncpy(dest, src, destlen);
}

static Bool_t
CheckMark(const char * lnbuf,
	  const char * leftmark, const char * rightmark,
	  char * retIndent, size_t szRetIndent,
	  char * retCommand, size_t szRetCommand,
	  char * retParamStr, size_t szRetParamStr)
{
    const char * b, * e, * tmp;
    if (!*lnbuf) return FALSE;
    b = lnbuf; e = lnbuf + strlen(lnbuf) - 1;
    if (!LocateMark(&b, &e, leftmark, rightmark)) return FALSE;
    tmp = b;
    while (b < e && isalnum(*b)) ++b;
    GetResult(retCommand, szRetCommand, tmp, b - tmp);
    if (!LocateMark(&b, &e, "(", ")")) *retParamStr = 0;
    else GetResult(retParamStr, szRetParamStr, b, e - b);
    for (b = lnbuf; isspace(*b); ++b);
    GetResult(retIndent, szRetIndent, lnbuf, b - lnbuf);
    return TRUE;
}

static int
TextWritter(FILE * outfp, char * lnbuf,
	    const char * replacedPrefix, const char * prefix)
{
    char * start, * cur; int len;
    if (!*replacedPrefix)
	return fputs(lnbuf, outfp) >= 0 ? 0 : -1;
    len = strlen(replacedPrefix);
    start = cur = lnbuf;
    while (*cur) {
	if (*cur != *replacedPrefix) ++cur;
	else if (strncmp(cur, replacedPrefix, len)) ++cur;
	else { /* An instance of replacedPrefix is found. */
	    *cur = 0;
	    if (fputs(start, outfp) < 0) return -1;
	    start = (cur += len);
	    if (fputs(prefix, outfp) < 0) return -1;
	}
    }
    return 0;
}

int
Frames(const char  * leftmark,
       const char  * rightmark,
       const char  * outDir,
       const char  * prefix,
       const char  * license,
       void        * cbData,
       int        (* cbFunc)(void * cbData,
			     FILE * outfp,
			     const char * indentstr,
			     const char * command,
			     const char * paramstr),
       int           numFrames,
       const char ** frameNames)
{
    const char ** curframefn;
    FILE * framefp, * outfp; Bool_t enabled;
    char outFName[256], lnbuf[4096];
    char indentStr[128], Command[128], ParamStr[128];
    char replacedPrefix[128];

    outfp = NULL;
    for (curframefn = frameNames;
	 curframefn - frameNames < numFrames; ++curframefn) {
	if (!(framefp = fopen(*curframefn, "r"))) goto errquit0;
	enabled = FALSE;
	while (fgets(lnbuf, sizeof(lnbuf), framefp)) {
	    if (!CheckMark(lnbuf, leftmark, rightmark,
			   indentStr, sizeof(indentStr),
			   Command, sizeof(Command),
			   ParamStr, sizeof(ParamStr))) {
		if (outfp && enabled &&
		    TextWritter(outfp, lnbuf, replacedPrefix, prefix) < 0)
		    goto errquit1;
		continue;
	    }
	    fputs(lnbuf, outfp);
	    if (!strcmp(Command, "open")) {
		if (outfp != NULL) fclose(outfp);
		snprintf(outFName, sizeof(outFName),
			 "%s/%s", outDir, ParamStr);
		if (!(outfp = fopen(outFName, "w"))) goto errquit1;
		enabled = TRUE;
	    } else if (!strcmp(Command, "prefix")) {
		if (ParamStr == NULL) replacedPrefix[0] = 0;
		else strncpy(replacedPrefix, ParamStr, sizeof(replacedPrefix));
	    } else if (!strcmp(Command, "license")) {
		if (outfp && fputs(license, outfp) < 0)
		    goto errquit1;
	    } else if (!strcmp(Command, "enable")) {
		enabled = TRUE;
	    } else if (!strcmp(Command, "disable")) {
		enabled = FALSE;
	    } else if (cbFunc(cbData, outfp,
			      indentStr, Command, ParamStr) < 0) {
		goto errquit1;
	    }
	}
	fclose(framefp);
    }
    if (outfp != NULL)  fclose(outfp);
    return 0;
 errquit1:
    if (outfp != NULL) fclose(outfp);
    fclose(framefp);
 errquit0:
    return -1;
}

int
Frame(const char * leftmark,
      const char * rightmark,
      const char * outDir,
      const char * prefix,
      const char * license,
      void       * cbData,
      int       (* cbFunc)(void       * cbData,
			   FILE       * outfp,
			   const char * indentstr,
			   const char * command,
			   const char * paramstr),
      const char * frameName)
{
    const char * frameNames[1];
    frameNames[0] = frameName;
    return Frames(leftmark, rightmark, outDir, prefix, license,
		  cbData, cbFunc, 1, frameNames);
}
