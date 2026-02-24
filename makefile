CC      = g++
CFLAGS  = -O3 -mavx -std=c++14 -w
LDFLAGS =

SOURCES = utils/utils.cpp containers/relation.cpp containers/offsets.cpp indices/hierarchicalindex.cpp indices/intervaltree.cpp indices/hint_m_subs+sort+ss+cm_novls.cpp indices/augmented_interval_tree.cpp indices/dit.cpp #main_dit_firas.cpp containers/offsets_templates.cpp containers/offsets.cpp indices/1dgrid.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: itree hint it_firas ait stream_firas_dit stream_ait

itree: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o indices/intervaltree.o main_intervaltree.cpp -o query_intervaltree.exec $(LDADD)

hint: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o containers/offsets.o indices/hierarchicalindex.o indices/hint_m_subs+sort+ss+cm_novls.o main_hint_m.cpp -o query_hint_m.exec $(LDADD)

it_firas: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o containers/offsets.o indices/intervaltree.o main_it_firas.cpp -o query_it_firas.exec $(LDADD)

ait: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o indices/augmented_interval_tree.o main_ait.cpp -o query_ait.exec $(LDADD)

stream_firas_dit: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o indices/dit.o main_stream_firas_dit.cpp -o query_stream_firas_dit.exec $(LDADD)

stream_ait: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) utils/utils.o containers/relation.o indices/augmented_interval_tree.o main_stream_ait.cpp -o query_stream_ait.exec $(LDADD)


.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf utils/*.o
	rm -rf containers/*.o
	rm -rf indices/*.o
	rm -rf query_intervaltree.exec
	rm -rf query_hint_m.exec
	rm -rf query_it_firas.exec
	rm -rf query_ait.exec
	rm -rf query_ait_dynamic.exec
	rm -rf query_dit_firas.exec
	rm -rf query_stream_firas_dit.exec
	rm -rf query_stream_ait.exec
	rm -rf query_btree.exec
	rm -rf query_btree_set.exec
