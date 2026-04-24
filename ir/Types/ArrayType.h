///
/// @file ArrayType.h
/// @brief 数组类型类，用于描述一维/多维数组
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2026-04-23
///
/// @copyright Copyright (c) 2026
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2026-04-23 <td>1.0     <td>copilot  <td>新建
/// </table>
///

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Type.h"

class ArrayType final : public Type {

public:
	///
	/// @brief 获取数组类型
	/// @param baseType 数组元素基础类型
	/// @param dims 数组维度，按从左到右顺序保存。形参数组首维可为0。
	/// @param parameterArray 是否为形参数组
	/// @return ArrayType*
	///
	static ArrayType * get(Type * baseType, std::vector<int32_t> dims, bool parameterArray = false);

	///
	/// @brief 获取数组元素基础类型
	/// @return const Type*
	///
	[[nodiscard]] Type * getElementType() const;

	///
	/// @brief 获取数组维度
	/// @return const std::vector<int32_t>&
	///
	[[nodiscard]] const std::vector<int32_t> & getDimensions() const;

	///
	/// @brief 兼容旧调用：获取数组维度
	/// @return const std::vector<int32_t>&
	///
	[[nodiscard]] const std::vector<int32_t> & getDims() const
	{
		return getDimensions();
	}

	///
	/// @brief 获取某一维的长度
	/// @param dim 维度索引
	/// @return int32_t
	///
	[[nodiscard]] int32_t getDimensionSize(size_t dim) const;

	///
	/// @brief 数组是否为形参数组
	/// @return true
	/// @return false
	///
	[[nodiscard]] bool isParameterArray() const;

	///
	/// @brief 获取数组类型字符串
	/// @return std::string
	///
	[[nodiscard]] std::string toString() const override;

	///
	/// @brief 获取数组占用内存大小
	/// @return int32_t
	///
	[[nodiscard]] int32_t getSize() const override;

	///
	/// @brief ArrayType内部的哈希键
	///
	struct ArrayKey {
		const Type * elementType = nullptr;
		std::vector<int32_t> dims;
		bool parameterArray = false;
	};

	struct ArrayKeyHasher {
		size_t operator()(const ArrayKey & key) const noexcept
		{
			size_t seed = std::hash<const Type *>{}(key.elementType);
			for (auto dim: key.dims) {
				seed ^= std::hash<int32_t>{}(dim) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
			seed ^= std::hash<bool>{}(key.parameterArray) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	struct ArrayKeyEqual {
		bool operator()(const ArrayKey & lhs, const ArrayKey & rhs) const noexcept
		{
			return lhs.elementType == rhs.elementType && lhs.dims == rhs.dims &&
				   lhs.parameterArray == rhs.parameterArray;
		}
	};

	///
	/// @brief 数组类型的存储表
	///
	static std::unordered_map<ArrayKey, ArrayType *, ArrayKeyHasher, ArrayKeyEqual> & getStorage();

	///
	/// @brief 构造函数
	///
	explicit ArrayType(Type * baseType, const std::vector<int32_t> & _dims);

private:
	Type * elementType = nullptr;
	std::vector<int32_t> dimensions;
	bool parameterArray = false;
};

inline std::unordered_map<ArrayType::ArrayKey, ArrayType *, ArrayType::ArrayKeyHasher, ArrayType::ArrayKeyEqual> &
ArrayType::getStorage()
{
	static std::unordered_map<ArrayKey, ArrayType *, ArrayKeyHasher, ArrayKeyEqual> storage;
	return storage;
}
