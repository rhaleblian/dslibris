#pragma once

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
/*!
A Page stores the flowed output from the XML parse and is equipped
to render a full left and right screen of text.

A Book contains a vector of Pages.
*/

class Book;
class Text;

class Page {
	public:
	Page(Book* b);
	~Page();
	void Cache(FILE *fp);
	void Draw();
	void Draw(Text *ts);
	u8*  GetBuffer() { return buf; }
	int  GetLength() { return length; }
	u8   SetBuffer(u8 *src, u16 len);    //! Copy src to buf for len bytes.
	int length;	                         //! Length of buf.
	int start;                           //! start index in a book's char buffer.
	int end;  	                         //! Ditto, for end char index. 

	private:
	void DrawNumber(Text *ts);
	Book* book;
	u8* buf;                             //! UTF-8 chars
};
