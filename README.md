#snapp-collector

The snapp collector is the backend of SNAPP (SNMP Network Analysis and
Presentation Package). Contained in this tarball.
The collector is a high performance multithreaded SNMP collector
with stability in mind.

For the ones in a hurry:
'./configure && make  && make installed'
Followed by a :
'snapp_configure.pl'
should leave you with a configured snapp collector (no init scripts yet).


##Documentaiton for users:

Snapp-collector contains a simple man page for advanced users, explaining
the running options for the collector. But not addressing any configuration
settings. These settings and the overall rational and architecture of
SNAPP can be found at the full user documentation that is available at:

Users can also generate the documentation of with the use of xsltproc
by running the makefile inside the documentation folder.




##Documentation for Developers:



###REMANNIG STUFF
Snapp collector is a threaded application
that uses two config files:
config and config.base

Both are XML documents.


###DEVELS:
check 'http://www.freesoftwaremagazine.com/books/agaal/automatically_writing_makefiles_with_autotools'
when autotools complain


