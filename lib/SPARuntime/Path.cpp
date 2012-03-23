/*
 * SPA - Systematic Protocol Analysis Framework
 */

#include "llvm/Support/MemoryBuffer.h"

#include "klee/ExprBuilder.h"
#include <expr/Parser.h>

#include <spa/Path.h>

namespace {
	typedef enum {
		START,
		PATH,
		SYMBOL_NAME,
		SYMBOL_ARRAY,
		SYMBOL_VALUE,
		TAG_KEY,
		TAG_VALUE,
		CONSTRAINTS,
		PATH_DONE
	} LoadState_t;
}

namespace SPA {
	std::ostream& operator<<( std::ostream &stream, const Path &path ) {
		stream << SPA_PATH_START << std::endl;
		for ( std::map<std::string, const klee::Array *>::const_iterator it = path.symbolNames.begin(), ie = path.symbolNames.end(); it != ie; it++ ) {
			stream << SPA_PATH_SYMBOL_START << std::endl;
			stream << it->first << std::endl;
			stream << it->second->name << std::endl;
			// Symbolic value.
			if ( path.symbolValues.count( it->first ) ) {
				for ( std::set<const klee::Array *>::iterator it2 = path.symbols.begin(), ie2 = path.symbols.end(); it2 != ie2; it2++ )
					stream << "array " << (*it2)->name << "[" << (*it2)->size << "] : w32 -> w8 = symbolic" << std::endl;
				stream << SPA_PATH_KLEAVER_START << std::endl;
				for ( std::vector<klee::ref<klee::Expr> >::const_iterator it2 = path.symbolValues.find( it->first )->second.begin(), ie2 = path.symbolValues.find( it->first )->second.end(); it2 != ie2; it2++ )
					stream << *it2 << std::endl;
				stream << SPA_PATH_KLEAVER_END << std::endl;
			} else {
				stream << "array " << it->second->name << "[" << it->second->size << "] : w32 -> w8 = symbolic" << std::endl;
			}
			stream << SPA_PATH_SYMBOL_END << std::endl;
		}

		for ( std::map<std::string, std::string>::const_iterator it = path.tags.begin(), ie = path.tags.end(); it != ie; it++ ) {
			stream << SPA_PATH_TAG_START << std::endl;
			stream << it->first << std::endl;
			stream << it->second << std::endl;
			stream << SPA_PATH_TAG_END << std::endl;
		}

		stream << SPA_PATH_CONSTRAINTS_START << std::endl;
		for ( std::set<const klee::Array *>::iterator it2 = path.symbols.begin(), ie2 = path.symbols.end(); it2 != ie2; it2++ )
			stream << "array " << (*it2)->name << "[" << (*it2)->size << "] : w32 -> w8 = symbolic" << std::endl;
		stream << SPA_PATH_KLEAVER_START << std::endl;
		for ( klee::ConstraintManager::constraint_iterator it = path.getConstraints()->begin(), ie = path.getConstraints()->end(); it != ie; it++)
			stream << *it << std::endl;
		stream << SPA_PATH_KLEAVER_END << std::endl;
		stream << SPA_PATH_CONSTRAINTS_END << std::endl;
		return stream << SPA_PATH_END << std::endl;
	}

	std::string cleanUpLine( std::string line ) {
		// Remove comments.
		line = line.substr( 0, line.find( SPA_PATH_COMMENT ) );
		// Remove trailing white space.
		line = line.substr( 0, line.find_last_not_of( SPA_PATH_WHITE_SPACE ) + 1 );
		// Remove leading white space.
		if ( line.find_first_not_of( SPA_PATH_WHITE_SPACE ) != line.npos )
			line = line.substr( line.find_first_not_of( SPA_PATH_WHITE_SPACE ), line.npos );
		
		return line;
	}

	#define changeState( from, to ) \
		assert( state == from && "Invalid path file." ); \
		state = to;

	std::set<Path *> Path::loadPaths( std::istream &pathFile ) {
		std::set<Path *> paths;

		LoadState_t state = START;
		Path *path = NULL;
		std::string symbolName, symbolArray, tagKey, tagValue, kQuery;
		while ( pathFile.good() ) {
			std::string line;
			getline( pathFile, line );
			line = cleanUpLine( line );
			if ( line.empty() )
				continue;

			if ( line == SPA_PATH_START ) {
				changeState( START, PATH );
				path = new Path();
			} else if ( line == SPA_PATH_SYMBOL_START ) {
				changeState( PATH, SYMBOL_NAME );
				symbolName = "";
				symbolArray = "";
				kQuery = "";
			} else if ( line == SPA_PATH_SYMBOL_END ) {
				changeState( SYMBOL_VALUE, PATH );
				assert( (! symbolName.empty()) && (! symbolArray.empty()) && "Invalid path file." );
				llvm::MemoryBuffer *MB = llvm::MemoryBuffer::getMemBuffer( kQuery );
				klee::ExprBuilder *Builder = klee::createDefaultExprBuilder();
				klee::expr::Parser *P = klee::expr::Parser::Create( "", MB, Builder );
				while ( klee::expr::Decl *D = P->ParseTopLevelDecl() ) {
					assert( ! P->GetNumErrors() && "Error parsing symbol value in path file." );
					if ( klee::expr::ArrayDecl *AD = dyn_cast<klee::expr::ArrayDecl>( D ) ) {
						path->symbols.insert( AD->Root );
						if ( symbolArray == AD->Root->name )
							path->symbolNames[symbolName] = AD->Root;
					} else if ( klee::expr::QueryCommand *QC = dyn_cast<klee::expr::QueryCommand>( D ) ) {
						path->symbolValues[symbolName] = QC->Constraints;
						delete D;
						break;
					}
				}
				delete P;
				delete Builder;
				delete MB;
			} else if ( line == SPA_PATH_TAG_START ) {
				changeState( PATH, TAG_KEY );
				tagKey = "";
				tagValue = "";
			} else if ( line == SPA_PATH_TAG_END ) {
				changeState( TAG_VALUE, PATH );
				assert( (! tagKey.empty()) && (! tagValue.empty()) && "Invalid path file." );
				path->tags[tagKey] = tagValue;
			} else if ( line == SPA_PATH_CONSTRAINTS_START ) {
				changeState( PATH, CONSTRAINTS );
				kQuery = "";
			} else if ( line == SPA_PATH_CONSTRAINTS_END ) {
				changeState( CONSTRAINTS, PATH_DONE );

				llvm::MemoryBuffer *MB = llvm::MemoryBuffer::getMemBuffer( kQuery );
				klee::ExprBuilder *Builder = klee::createDefaultExprBuilder();
				klee::expr::Parser *P = klee::expr::Parser::Create( "", MB, Builder );
				while ( klee::expr::Decl *D = P->ParseTopLevelDecl() ) {
					assert( ! P->GetNumErrors() && "Error parsing constraints in path file." );
					if ( klee::expr::QueryCommand *QC = dyn_cast<klee::expr::QueryCommand>( D ) ) {
						path->constraints = new klee::ConstraintManager( QC->Constraints );
						delete D;
						break;
					}
				}
				delete P;
				delete Builder;
				delete MB;
			} else if ( line == SPA_PATH_END ) {
				changeState( PATH_DONE, START );
				paths.insert( path );
			} else {
				switch ( state ) {
					case SYMBOL_NAME : {
						assert( symbolName.empty() && "Invalid path file." );
						symbolName = line;
						changeState( SYMBOL_NAME, SYMBOL_ARRAY );
					} break;
					case SYMBOL_ARRAY : {
						assert( symbolArray.empty() && "Invalid path file." );
						symbolArray = line;
						changeState( SYMBOL_ARRAY, SYMBOL_VALUE );
					} break;
					case SYMBOL_VALUE : {
						kQuery += " " + line;
					} break;
					case TAG_KEY : {
						assert( tagKey.empty() && "Invalid path file." );
						tagKey = line;
						changeState( TAG_KEY, TAG_VALUE );
					} break;
					case TAG_VALUE : {
						assert( tagValue.empty() && "Invalid path file." );
						tagValue = line;
					} break;
					case CONSTRAINTS : {
						kQuery += " " + line;
					} break;
					default : {
						assert( false && "Invalid path file." );
					} break;
				}
			}
		}

		std::cerr << "Loaded " << paths.size() << " paths." << std::endl;

		return paths;
	}
}
