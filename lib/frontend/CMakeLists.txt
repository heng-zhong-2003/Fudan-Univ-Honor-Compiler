find_package(FLEX)
find_package(BISON)
FLEX_TARGET(lexer lexer.lex ${PROJECT_BINARY_DIR}/include/frontend/lexer.c)
BISON_TARGET(parser parser.yacc ${PROJECT_BINARY_DIR}/include/frontend/parser.c COMPILE_FLAGS -v)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

aux_source_directory(. FRONTEND_SRCS)
add_library(frontend OBJECT 
  ${FRONTEND_SRCS}
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS}
)

# aux_source_directory(. FRONTEND_SRCS)
# add_library(frontend OBJECT ${FRONTEND_SRCS})
