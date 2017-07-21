
add_sources(libopenage
	continuous.cpp
	curve.cpp
	discrete.cpp
	iterator.cpp
	map.cpp
	map_filter_iterator.cpp
	queue.cpp
	queue_filter_iterator.cpp
)

pxdgen(
	continuous.h
	curve.h
	discrete.h
	iterator.h
	map.h
	map_filter_iterator.h
	queue.h
	queue_filter_iterator.h
)

add_subdirectory(events)
add_subdirectory(demo)
add_subdirectory(internal)
add_subdirectory(test)
