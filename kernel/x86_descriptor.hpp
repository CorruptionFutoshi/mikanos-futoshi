#pragma once

enum class DescriptorType {
	kUpper8Bytes	= 0,
	kLDT		= 2,
	kTSSAvailable	= 9,
	kTSSBusy	= 11,
	kCallGate	= 12,
	kInterruptGate	= 14,
	kTrapGate	= 15,

	// enum class doesn't execute implicit type conversion. so if there are enumerators which have same value, it's no problem.
	kReadWrite	= 2,
	kExecuteRead	= 10,
};
