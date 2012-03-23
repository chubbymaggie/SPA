/*
 * SPA - Systematic Protocol Analysis Framework
 */

#include <sstream>

#include "llvm/Instructions.h"

#include "spa/CG.h"

#include "spa/CFG.h"


using namespace llvm;

namespace SPA {
	CFG::CFG( Module *module ) {
		// Iterate functions.
		for ( Module::iterator mit = module->begin(), mie = module->end(); mit != mie; mit++ ) {
			Function &fn = *mit;
			// Iterate basic blocks.
			for ( Function::iterator fit = fn.begin(), fie = fn.end(); fit != fie; fit++ ) {
				BasicBlock &bb = *fit;
				// Connect across basic blocks.
				// Iterate successors.
				TerminatorInst *ti = bb.getTerminator();
				unsigned int ns = ti->getNumSuccessors();
				for ( unsigned int i = 0; i < ns; i++ ) {
					// Add entry (even if with no successors) for the current predecessor.
					predecessors[&(bb.front())];
					successors[ti];
					predecessors[&(ti->getSuccessor( i )->front())].insert( ti );
					successors[ti].insert( &(ti->getSuccessor( i )->front()) );
				}
				// Connect within basic block.
				// Iterate instructions.
				BasicBlock::iterator bbit = bb.begin(), bbie = bb.end();
				instructions.insert( &(*bbit) );
				Instruction *prevInst = &(*(bbit++));
				for ( ; bbit != bbie; bbit++ ) {
					Instruction *inst = &(*bbit);
					instructions.insert( inst );
					predecessors[inst];
					successors[inst];
					predecessors[inst].insert( prevInst );
					successors[prevInst].insert( inst );
					prevInst = inst;
				}
			}
		}
	}

	CFG::iterator CFG::begin() {
		return instructions.begin();
	}
	
	CFG::iterator CFG::end() {
		return instructions.end();
	}
	
	const std::set<Instruction *> &CFG::getSuccessors( Instruction *instruction ) {
		return successors[instruction];
	}

	const std::set<Instruction *> &CFG::getPredecessors( Instruction *instruction ) {
		return predecessors[instruction];
	}

	void CFG::dump( std::ostream &dotFile, std::map<InstructionFilter *, std::string> &annotations ) {
		// Generate CFG DOT file.
		dotFile<< "digraph CFG {" << std::endl;

		// Add all instructions.
		for ( iterator it = begin(), ie = end(); it != ie; it++ ) {
			Instruction *inst = *it;
			std::stringstream attributes;
			// Annotate entry / exit points.
			if ( getSuccessors( inst ).empty() )
				attributes << "shape = \"doublecircle\"";
			else if ( inst == &(inst->getParent()->getParent()->getEntryBlock().front()) )
				attributes << "shape = \"box\"";
			else
				attributes << "shape = \"oval\"";
			// Annotate source line.
			attributes << " label = \"" << inst->getDebugLoc().getLine() << "\"";
			// Add user annotations.
			for ( std::map<InstructionFilter *, std::string>::iterator it = annotations.begin(), ie = annotations.end(); it != ie; it++ )
				if ( it->first->checkInstruction( inst ) )
					attributes << " " << it->second;

			dotFile << "	subgraph cluster_" << inst->getParent()->getParent()->getName().str() << " {" << std::endl
				<< "		label = \"" << inst->getParent()->getParent()->getName().str() << "\";" << std::endl
				<< "		n" << ((long) inst) << " [" << attributes.str() << "];" << std::endl
				<< "	}" << std::endl;
		}
		// Add edges.
		// Successors.
		dotFile << "	edge [color = \"green\"];" << std::endl;
		for ( iterator it1 = begin(), ie1 = end(); it1 != ie1; it1++ )
			for ( iterator it2 = getSuccessors( *it1 ).begin(), ie2 = getSuccessors( *it1 ).end(); it2 != ie2; it2++ )
				dotFile << "	n" << ((unsigned long) *it1) << " -> n" << ((unsigned long) *it2) << ";" << std::endl;
		// Callers.
		CG cg = CG( *this );
		dotFile << "	edge [color = \"blue\"];" << std::endl;
		for ( CG::iterator it1 = cg.begin(), ie1 = cg.end(); it1 != ie1; it1++ ) {
			llvm::Function *fn = *it1;
			for ( std::set<llvm::Instruction *>::iterator it2 = cg.getCallers( fn ).begin(), ie2 = cg.getCallers( fn ).end(); it2 != ie2; it2++ ) {
				if ( fn == NULL )
					dotFile << "	IndirectFunction [label = \"*\" shape = \"box\"]" << std::endl
						<< "	n" << ((unsigned long) *it2) << " -> IndirectFunction;" << std::endl;
				else if ( ! fn->empty() )
					dotFile << "	n" << ((unsigned long) *it2) << " -> n" << ((unsigned long) &(fn->getEntryBlock().front())) << ";" << std::endl;
				else
					dotFile << "	n" << ((unsigned long) fn) << " [label = \"" << fn->getName().str() << "\" shape = \"box\"]" << std::endl
						<< "	n" << ((unsigned long) *it2) << " -> n" << ((unsigned long) fn) << ";" << std::endl;
			}
		}

		dotFile << "}" << std::endl;
	}
}
