# wrapper for C++ sockets on Unix and Windows
#   Copyright (C) 2002
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

CXX = g++
CXXFLAGS = -Wall -ansi -pedantic -g 

ifeq ($(shell uname),SunOS)
  LIBS = -lsocket -lnsl
endif

AUX=PracticalSocket.cc  
AUX_H=PracticalSocket.hh StringFunctions.hh FileDesc.hh 

all: ClientLibrary MetadataManager FileServer  

ClientLibrary: ClientLibrary.cc $(AUX) $(AUX_H)
	$(CXX) $(CXXFLAGS) -o  ClientLibrary ClientLibrary.cc $(AUX) $(LIBS)

MetadataManager: MetadataManager.cc MetadataManager.hh $(AUX) $(AUX_H) 
	$(CXX) -D_GNU_SOURCE -o MetadataManager MetadataManager.cc $(AUX) $(LIBS) -lpthread

FileServer: FileServer.cc $(AUX) $(AUX_H) 
	$(CXX) -o FileServer FileServer.cc $(AUX) $(LIBS) -lpthread 

clean:
	$(RM) ClientLibrary MetadataManager FileServer 

TCPEchoClient: TCPEchoClient.cc PracticalSocket.cc PracticalSocket.hh
	$(CXX) $(CXXFLAGS) -o TCPEchoClient TCPEchoClient.cc PracticalSocket.cc $(LIBS)

TCPEchoServer: TCPEchoServer_Thread.cc PracticalSocket.cc PracticalSocket.hh
	$(CXX) -D_GNU_SOURCE -o TCPEchoServer-Thread TCPEchoServer_Thread.cc PracticalSocket.cc $(LIBS) -lpthread
  
