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
#include  <stdio.h>
#include  <stdlib.h>
#include  "AutoTests.h"

void TestCharSet(FILE * fp);

int
main(void)
{
    FILE * fp;

    if (!(fp = fopen("AutoTests.log", "w"))) {
	fprintf(stderr, "Open AutoTests.log for written failed.\n");
	exit(-1);
    }
    TestCharSet(fp);
    fclose(fp);
    return 0;
}

void
coco_assert_failed(const char * fname, int lineno)
{
    /* Breakpoint can be set here. */
    fprintf(stderr, "Assert failed at: %s#%d.\n", fname, lineno);
    exit(-1);
}
