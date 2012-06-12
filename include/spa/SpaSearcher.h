/*
 * SPA - Systematic Protocol Analysis Framework
 */

#ifndef __SPASEARCHER_H__
#define __SPASEARCHER_H__

#include <klee/Searcher.h>

#include <spa/InstructionFilter.h>
#include <spa/FilteringEventHandler.h>


namespace SPA {
	class SpaSearcher : public klee::Searcher {
	private:
		InstructionFilter *filter;
		std::list<klee::ExecutionState *> states;
		std::list<FilteringEventHandler *> filteringEventHandlers;
		unsigned long statesDequeued;
		unsigned long statesFiltered;

	public:
		explicit SpaSearcher( InstructionFilter *_filter )
			: filter( _filter ), statesDequeued( 0 ), statesFiltered( 0 ) { };
		void addFilteringEventHandler( FilteringEventHandler *handler ) { filteringEventHandlers.push_back( handler ); }
		~SpaSearcher() { };

		klee::ExecutionState &selectState();
		void update( klee::ExecutionState *current, const std::set<klee::ExecutionState *> &addedStates, const std::set<klee::ExecutionState *> &removedStates );
		bool empty() { return states.empty(); }
		void printName( std::ostream &os ) {
			os << "SpaSearcher" << std::endl;
		}
	};
}

#endif // #ifndef __SPASEARCHER_H__
