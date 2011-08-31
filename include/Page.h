/*
 Copyright (C) 2007-2009 Ray Haleblian
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
 To contact the copyright holder: rayh23@sourceforge.net
 */

#ifndef PAGE_H
#define PAGE_H

#include <nds.h>
#include "Book.h"
#include "Text.h"

class Page {
	class Book *book;
	void DrawNumber(Text *ts);

 public:
	u8 *buf;       //! UTF-8 chars, allocated per-page at parse time, to exact length.
	int length;    //! Length of buf.
	int start;     //! In a book-long char buffer, where would i begin? 
	int end;       //! Ditto, for end char.
	Page(Book *b);
	Page(Book *b, Text *t);
	~Page();
	u8 SetBuffer(u8 *src, u16 len); //! Copy src to buf for len bytes.
//	void Draw();
	void Draw(Text *ts);
};

#endif
