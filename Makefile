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
CXXFLAGS = -ansi -pedantic -g

ifeq ($(shell uname),SunOS)
  LIBS = -lsocket -lnsl
endif

AUX_H=PracticalSocket.hh StringFunctions.hh FileDesc.hh net_addresses.hh data_types.hh mt_data_types.hh net_addresses.hh 

all: test MetadataManager FileServer  copy 

test1: test_client1.cc pfs.hh FileDesc.hh PracticalSocket.cc ClientCache.cc pfs.cc 
	$(CXX) $(CXXFLAGS) -o test_client1 PracticalSocket.cc  ClientCache.cc pfs.cc FileDesc.cc test_client1.cc -lpthread 

test2: test_client2.cc pfs.hh FileDesc.hh PracticalSocket.cc ClientCache.cc pfs.cc 
	$(CXX) $(CXXFLAGS) -o test_client2 PracticalSocket.cc  ClientCache.cc pfs.cc FileDesc.cc test_client2.cc -lpthread 

#PracticalSocket: PracticalSocket.cc 
#	$(CXX) $(CXXFLAGS) -o PracticalSocket.cc -lpthread


#ClientLibrary: ClientLibrary.cc ClientCache PracticalSocket $(AUX_H)
#	$(CXX) $(CXXFLAGS) -o  PracticalSocket.cc ClientCache.cc ClientLibrary.cc -lpthread

MetadataManager: MetadataManager.cc MetadataManager.hh PracticalSocket.cc $(AUX_H) 
	$(CXX) -D_GNU_SOURCE -o MetadataManager MetadataManager.cc  PracticalSocket.cc $(LIBS) -lpthread

FileServer: FileServer.cc PracticalSocket.cc $(AUX_H) 
	$(CXX) -o FileServer FileServer.cc PracticalSocket.cc $(LIBS) -lpthread 

clean:
	$(RM) test_client MetadataManager FileServer 

TCPEchoClient: TCPEchoClient.cc PracticalSocket.cc PracticalSocket.hh
	$(CXX) $(CXXFLAGS) -o TCPEchoClient TCPEchoClient.cc PracticalSocket.cc $(LIBS)

TCPEchoServer: TCPEchoServer_Thread.cc PracticalSocket.cc PracticalSocket.hh
	$(CXX) -D_GNU_SOURCE -o TCPEchoServer-Thread TCPEchoServer_Thread.cc PracticalSocket.cc $(LIBS) -lpthread
 
copy: FileServer 
	cp FileServer fs0
	cp FileServer fs1
	cp FileServer fs2
	cp FileServer fs3
	cp FileServer fs4

