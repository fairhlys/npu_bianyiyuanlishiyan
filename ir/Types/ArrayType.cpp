///
/// @file ArrayType.cpp
/// @brief 数组类型类，用于描述一维/多维数组
///

#include "ArrayType.h"

ArrayType::ArrayType(Type * baseType, const std::vector<int32_t> & _dims)
	: Type(ArrayTyID), elementType(baseType), dimensions(_dims)
{}

Type * ArrayType::getElementType() const
{
	return elementType;
}

const std::vector<int32_t> & ArrayType::getDimensions() const
{
	return dimensions;
}

int32_t ArrayType::getDimensionSize(size_t dim) const
{
	if (dim >= dimensions.size()) {
		return 0;
	}
	return dimensions[dim];
}

bool ArrayType::isParameterArray() const
{
	return !dimensions.empty() && dimensions.front() == 0;
}

std::string ArrayType::toString() const
{
	std::string typeStr = elementType->toString();
	if (parameterArray) {
		typeStr += "*";
		return typeStr;
	}
	if (!dimensions.empty()) {
		typeStr += " ";
	}
	for (auto dim: dimensions) {
		typeStr += "[" + std::to_string(dim) + "]";
	}
	return typeStr;
}

int32_t ArrayType::getSize() const
{
	if (parameterArray) {
		return 4;
	}
	if (dimensions.empty()) {
		return elementType->getSize();
	}
	int64_t count = 1;
	for (auto dim: dimensions) {
		if (dim <= 0) {
			return 4;
		}
		count *= dim;
	}
	return (int32_t) (count * elementType->getSize());
}

ArrayType * ArrayType::get(Type * baseType, std::vector<int32_t> dims, bool parameterArray)
{
	ArrayKey key{baseType, std::move(dims), parameterArray};
	auto & storage = getStorage();
	auto iter = storage.find(key);
	if (iter != storage.end()) {
		return iter->second;
	}

	auto * type = new ArrayType(baseType, key.dims);
	type->parameterArray = parameterArray;
	storage.emplace(std::move(key), type);
	return type;
}
