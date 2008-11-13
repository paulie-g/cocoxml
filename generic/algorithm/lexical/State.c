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
#include  "lexical/State.h"
#include  "lexical/Action.h"
#include  "lexical/CharSet.h"

static void
CcState_Construct(CcObject_t * self, va_list ap)
{
    CcState_t * ccself = (CcState_t *)self;
    ccself->firstAction = NULL;
    ccself->endOf = NULL;
    ccself->ctx = 0;
}
static void
CcState_Destruct(CcObject_t * self)
{
    CcAction_t * cur, * next;
    CcState_t * ccself = (CcState_t *)self;

    for (cur = ccself->firstAction; cur; cur = next) {
	next = cur->next;
	CcAction_Destruct(cur);
    }
    CcObject_Destruct(self);
}

static CcObjectType_t StateType = {
    sizeof(CcState_t), "State", CcState_Construct, CcState_Destruct
};
const CcObjectType_t * state = &StateType;

void
CcState_AddAction(CcState_t * self, CcAction_t * act)
{
    int actSize = CcAction_ShiftSize(act);
    CcAction_t * lasta = NULL, * a = self->firstAction;

    /* Collect bigger classes at the beginning gives better performance. */
    while (a != NULL && actSize <= CcAction_ShiftSize(a)) {
	lasta = a; a = a->next;
    }
    act->next = a;
    if (a == self->firstAction) self->firstAction = act;
    else  lasta->next = act;
}

void
CcState_DetachAction(CcState_t * self, CcAction_t * act)
{
    CcAction_t * lasta = NULL, * a = self->firstAction;
    while (a != NULL && a != act) { lasta = a; a = a->next; }
    if (a != NULL) {
	if (a == self->firstAction) self->firstAction = a->next;
	else lasta->next = a->next;
    }
}

int
CcState_MeltWith(CcState_t * self, CcState_t * s)
{
    CcAction_t * action;
    for (action = s->firstAction; action != NULL; action = action->next)
	CcState_AddAction(self, CcAction_Clone(action));
    return 0;
}

static void
CcState_SplitActions(CcState_t * self, CcAction_t * a, CcAction_t * b)
{
    CcAction_t * c;
    CcCharSet_t * seta, * setb, * setc;
    CcTransition_t trans;

    seta = CcAction_GetShift(a);
    setb = CcAction_GetShift(b);
    if (CcCharSet_Equals(seta, setb)) {
	CcAction_AddTargets(a, b);
	CcState_DetachAction(self, b);
    } else if (CcCharSet_Includes(seta, setb)) {
	setc = CcCharSet_Clone(seta); CcCharSet_Subtract(setc, setb);
	CcAction_AddTargets(b, a);
	CcAction_SetShift(a, setc);
	CcCharSet_Destruct(setc);
    } else if (CcCharSet_Includes(setb, seta)) {
	setc = CcCharSet_Clone(setb); CcCharSet_Subtract(setc, seta);
	CcAction_AddTargets(a, b);
	CcAction_SetShift(b, setc);
	CcCharSet_Destruct(setc);
    } else {
	setc = CcCharSet_Clone(seta); CcCharSet_And(setc, setb);
	CcCharSet_Subtract(seta, setc);
	CcCharSet_Subtract(setb, setc);
	CcAction_SetShift(a, seta);
	CcAction_SetShift(b, setb);

	CcTransition_Clone(&trans, &a->trans);
	CcTransition_SetCharSet(&trans, setc);
	CcTransition_SetCode(&trans, trans_normal);
	c = CcAction(&trans);
	CcTransition_Destruct(&trans);
	CcAction_AddTargets(c, a);
	CcAction_AddTargets(c, b);

	CcState_AddAction(self, c);
	CcCharSet_Destruct(setc);
    }
}

CcsBool_t
CcState_MakeUnique(CcState_t * self)
{
    CcAction_t * a, * b;
    CcsBool_t changed = FALSE;

    for (a = self->firstAction; a != NULL; a = a->next)
	for (b = a->next; b != NULL; b = b->next)
	    if (CcAction_Overlap(a, b)) {
		CcState_SplitActions(self, a, b);
		changed = TRUE;
	    }
    return changed;
}
