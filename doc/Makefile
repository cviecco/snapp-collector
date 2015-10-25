#the location of the templates
#BASE_TEMPLATE_LOC=/tmp/docbook-xsl-1.75.2/
BASE_TEMPLATE_LOC=http://docbook.sourceforge.net/release/xsl/1.75.2/


all: index.html manual.pdf manual.txt 

index.html: docbook_doc.xml
	xsltproc $(BASE_TEMPLATE_LOC)/html/chunk.xsl docbook_doc.xml

manual.txt: single.html
	elinks -dump single.html > manual.txt

single.html: docbook_doc.xml
	xsltproc --output single.html $(BASE_TEMPLATE_LOC)/html/docbook.xsl docbook_doc.xml

manual.pdf: manual.fo
	fop manual.fo manual.pdf

manual.fo: docbook_doc.xml
	xsltproc --output manual.fo $(BASE_TEMPLATE_LOC)/fo/docbook.xsl docbook_doc.xml


clean:
	rm -f *.html
	rm -f manual.pdf
	rm -f manual.fo
	rm -f manual.txt	
