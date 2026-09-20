PROJECT(hatnmediatests)

# A module that owns tests only, like clientservertests: it has no library sources (no src/ directory, so
# hatn treats it as header only) and exists so that the tests of media that need crypt and a crypto
# plugin have a place to live without media itself depending on crypt.
SET (HEADERS
)

SET (SOURCES
)

BUILD_HATN_MODULE()
