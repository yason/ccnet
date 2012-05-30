
Ccnet is a framework for writing networked applications in C. It
provides the following basic services:

1. Peer identification
2. Connection Management
3. Service invocation
4. Message sending

In ccnet network, there are two types of nodes, i.e., client and server.
Server has the following functions:

1. User management
2. Group management
3. Cluster management

Dependency
==========

The following packages are required to build libsearpc:

    json-glib >= 0.10.2
    valac >= 0.8
    libsearpc
    libmysqlclient-dev for compiling ccnet server
    libzdb >= 2.10.2

Compile
=======

To compile the client components, just

    ./autogen.sh; ./configure; make; make install

To also compile the server components, use

    ./configure --enable-server
