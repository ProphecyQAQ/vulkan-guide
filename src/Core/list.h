#pragma once

//
// Simple single-linked list template.
//
template <class ElementType> class List
{
public:

	ElementType			element;
	List<ElementType>*	next;

	// Constructor.

	[[nodiscard]] List(const ElementType &InElement, List<ElementType>* InNext = nullptr)
	{
		element = InElement;
		next = InNext;
	}
};
