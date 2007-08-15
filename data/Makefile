
all: jefferson.xhtml

jefferson.xhtml: jefferson.txt
	tidy -asxhtml jefferson.txt > jefferson.xhtml

coe.xhtml: coe.htm
	tidy -asxhtml coe.htm > coe.xhtml

clean:
	rm *.xhtml

