/*---- open(Scanner.c) S ----*/
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
/*---- enable ----*/
#include  <stdlib.h>
#include  <string.h>
#include  "Scanner.h"

Token_t *
Token(Token_t * self)
{
    if (!self && !(self = malloc(sizeof(Token_t)))) return NULL;
    self->kind = 0;
    self->pos = 0;
    self->col = 0;
    self->line = 0;
    self->val = NULL;
    self->next = NULL;
    return self;
}

void
Token_Destruct(Token_t * self)
{
    if (self->val) free(self->val);
}

typedef struct {
    int key, val;
}  Char2State_t;

static const Char2State_t c2sArr[] = {
    /*---- chars2states ----*/
};
static const int c2sNum = sizeof(c2sArr) / sizeof(c2sArr[0]);

static int
c2sCmp(const void * key, const void * c2s)
{
    return *((const int *)key) - ((const Char2State_t *)c2s)->key;
}
static int
Char2State(int chr)
{
    Char2State_t * c2s;

    c2s = bsearch(&chr, c2sArr, c2sNum, sizeof(Char2State_t), c2sCmp);
    return c2s ? c2s->val : 0;
}

typedef struct {
    const char * key;
    int val;
}  Identifier2KWKind_t;

static const Identifier2KWKind_t i2kArr[] = {
    /*----  identifiers2keywordkinds ----*/
};
static const int i2kNum = sizeof(i2kArr) / sizeof(i2kArr[0]);

static int
i2kCmp(const void * key, const void * i2k)
{
    return strcmp((const char *)key, ((const Identifier2KWKind_t *)i2k)->key);
}

static int
Identifier2KWKind(const char * key, int defaultVal)
{
    Identifier2KWKind_t * i2k;

    i2k = bsearch(key, i2kArr, i2kNum, sizeof(Identifier2KWKind_t), i2kCmp);
    return i2k ? i2k->val : defaultVal;
}

Buffer_t *
Buffer(Buffer_t * self, FILE * fp)
{
}

void
Buffer_Destruct(Buffer_t * self)
{
}

int
Buffer_Read(Buffer_t * self)
{
}

int
Buffer_Peek(Buffer_t * self)
{
}

const char *
Buffer_GetString(Buffer_t * self, int start)
{
}

int
Buffer_GetPos(Buffer_t * self)
{
}

void
Buffer_SetPos(Buffer_t * self, int pos)
{
}
