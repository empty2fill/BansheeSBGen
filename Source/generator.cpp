#include "common.h"

std::string getInteropCppVarType(const std::string& typeName, ParsedType type, int flags)
{
	if (isArray(flags))
	{
		if (isOutput(flags))
			return "MonoArray**";
		else
			return "MonoArray*";
	}

	switch (type)
	{
	case ParsedType::Builtin:
	case ParsedType::Enum:
		if (isOutput(flags))
			return typeName + "*";
		else
			return typeName;
	case ParsedType::Struct:
		return typeName + "*";
	case ParsedType::String:
	case ParsedType::WString:
		if (isOutput(flags))
			return "MonoString**";
		else
			return "MonoString*";
	default:
		if (isOutput(flags))
			return "MonoObject**";
		else
			return "MonoObject*";
	}
}

std::string getCppVarType(const std::string& typeName, ParsedType type)
{
	if (type == ParsedType::Resource)
		return "ResourceHandle<" + typeName + ">";
	else if (type == ParsedType::SceneObject || type == ParsedType::Component)
		return "GameObjectHandle<" + typeName + ">";
	else if (type == ParsedType::Class)
		return "SPtr<" + typeName + ">";
	else
		return typeName;
}

bool isPlainStruct(ParsedType type, int flags)
{
	return type == ParsedType::Struct && !isArray(flags);
}

std::string getCSVarType(const std::string& typeName, ParsedType type, int flags, bool paramPrefixes,
	bool arraySuffixes, bool forceStructAsRef)
{
	std::stringstream output;

	if (paramPrefixes && isOutput(flags))
	{
		if (isPlainStruct(type, flags))
			output << "ref ";
		else
			output << "out ";
	}
	else if (forceStructAsRef && (isPlainStruct(type, flags)))
		output << "ref ";

	output << typeName;

	if (arraySuffixes && isArray(flags))
		output << "[]";

	return output.str();
}

std::string getAsCppArgument(const std::string& name, ParsedType type, int flags, const std::string& methodName)
{
	auto getArgumentPlain = [&](bool isPtr)
	{
		assert(!isSrcRHandle(flags) && !isSrcGHandle(flags) && !isSrcSPtr(flags));

		if (isSrcPointer(flags))
			return (isPtr ? "" : "&") + name;
		else if (isSrcReference(flags) || isSrcValue(flags))
			return (isPtr ? "*" : "") + name;
		else
		{
			errs() << "Unsure how to pass parameter \"" << name << "\" to method \"" << methodName << "\".\n";
			return name;
		}
	};

	switch (type)
	{
	case ParsedType::Builtin:
	case ParsedType::Enum: // Input type is either value or pointer depending or output or not
		return getArgumentPlain(isOutput(flags));
	case ParsedType::Struct: // Input type is always a pointer
		return getArgumentPlain(true);
	case ParsedType::String:
	case ParsedType::WString: // Input type is always a value
		return getArgumentPlain(false);
	case ParsedType::Component: // Input type is always a handle
	case ParsedType::SceneObject:
	case ParsedType::Resource:
	{
		if (isSrcPointer(flags))
			return name + ".get()";
		else if (isSrcReference(flags) || isSrcValue(flags))
			return "*" + name;
		else if (isSrcSPtr(flags))
			return name + ".getInternalPtr()";
		else if (isSrcRHandle(flags) || isSrcGHandle(flags))
			return name;
		else
		{
			errs() << "Unsure how to pass parameter \"" << name << "\" to method \"" << methodName << "\".\n";
			return name;
		}
	}
	case ParsedType::Class: // Input type is always a SPtr
	{
		assert(!isSrcRHandle(flags) && !isSrcGHandle(flags));

		if (isSrcPointer(flags))
			return name + ".get()";
		else if (isSrcReference(flags) || isSrcValue(flags))
			return "*" + name;
		else if (isSrcSPtr(flags))
			return name;
		else
		{
			errs() << "Unsure how to pass parameter \"" << name << "\" to method \"" << methodName << "\".\n";
			return name;
		}

	}
	default: // Some object type
		assert(false);
		return "";
	}
}

std::string getScriptInteropType(const std::string& name)
{
	auto iterFind = cppToCsTypeMap.find(name);
	if (iterFind == cppToCsTypeMap.end())
		errs() << "Type \"" << name << "\" referenced as a script interop type, but no script interop mapping found. Assuming default type name.\n";

	bool isValidInteropType = iterFind->second.type != ParsedType::Builtin &&
		iterFind->second.type != ParsedType::Enum &&
		iterFind->second.type != ParsedType::String &&
		iterFind->second.type != ParsedType::WString;

	if (!isValidInteropType)
		errs() << "Type \"" << name << "\" referenced as a script interop type, but script interop object cannot be generated for this object type.\n";

	return "Script" + name;
}

bool isValidStructType(UserTypeInfo& typeInfo, int flags)
{
	if (isOutput(flags) || isArray(flags))
		return false;

	if (typeInfo.type == ParsedType::Builtin || typeInfo.type == ParsedType::Enum || typeInfo.type == ParsedType::Struct)
		return true;

	return false;
}

MethodInfo findUnusedCtorSignature(const ClassInfo& classInfo)
{
	int numBools = 1;
	while (true)
	{
		bool isSignatureValid = true;
		for (auto& entry : classInfo.ctorInfos)
		{
			if ((int)entry.paramInfos.size() != numBools)
				continue;

			bool isCtorValid = false;
			for (auto& paramInfo : entry.paramInfos)
			{
				if (paramInfo.type != "bool")
				{
					isCtorValid = true;
					break;
				}
			}

			if (!isCtorValid)
			{
				isSignatureValid = false;
				break;
			}
		}

		if (isSignatureValid)
			break;

		numBools++;
	}

	MethodInfo output;
	output.sourceName = classInfo.name;
	output.scriptName = classInfo.name;
	output.flags = (int)MethodFlags::Constructor;

	for (int i = 0; i < numBools; i++)
	{
		VarInfo paramInfo;
		paramInfo.name = "__dummy" + std::to_string(i);
		paramInfo.type = "bool";
		paramInfo.flags = (int)TypeFlags::Builtin;

		output.paramInfos.push_back(paramInfo);
	}

	return output;
}

void gatherIncludes(const std::string& typeName, int flags, std::unordered_map<std::string, IncludeInfo>& output)
{
	UserTypeInfo typeInfo = getTypeInfo(typeName, flags);
	if (typeInfo.type == ParsedType::Class || typeInfo.type == ParsedType::Component ||
		typeInfo.type == ParsedType::SceneObject || typeInfo.type == ParsedType::Resource ||
		typeInfo.type == ParsedType::Enum)
	{
		auto iterFind = output.find(typeName);
		if (iterFind == output.end())
		{
			// If enum or passed by value we need to include the header for the source type
			bool sourceInclude = typeInfo.type == ParsedType::Enum || isSrcValue(flags);

			output[typeName] = IncludeInfo(typeName, typeInfo, sourceInclude);
		}
	}
}

void gatherIncludes(const MethodInfo& methodInfo, std::unordered_map<std::string, IncludeInfo>& output)
{
	bool returnAsParameter = false;
	if (!methodInfo.returnInfo.type.empty())
		gatherIncludes(methodInfo.returnInfo.type, methodInfo.returnInfo.flags, output);

	for (auto I = methodInfo.paramInfos.begin(); I != methodInfo.paramInfos.end(); ++I)
		gatherIncludes(I->type, I->flags, output);
}

void gatherIncludes(const ClassInfo& classInfo, std::unordered_map<std::string, IncludeInfo>& output)
{
	for (auto& methodInfo : classInfo.ctorInfos)
		gatherIncludes(methodInfo, output);

	for (auto& methodInfo : classInfo.methodInfos)
		gatherIncludes(methodInfo, output);
}

void postProcessFileInfos()
{
	// Inject external methods into their appropriate class infos
	auto findClassInfo = [](const std::string& name) -> ClassInfo*
	{
		for (auto& fileInfo : outputFileInfos)
		{
			for (auto& classInfo : fileInfo.second.classInfos)
			{
				if (classInfo.name == name)
					return &classInfo;
			}
		}

		return nullptr;
	};

	for (auto& entry : externalMethodInfos)
	{
		ClassInfo* classInfo = findClassInfo(entry.first);
		if (classInfo == nullptr)
			continue;

		for (auto& method : entry.second.methods)
		{
			if (((int)method.flags & (int)MethodFlags::Constructor) != 0)
			{
				if (method.returnInfo.type.size() == 0)
				{
					errs() << "Found an external constructor \"" << method.sourceName << "\" with no return value, skipping.\n";
					continue;
				}

				if (method.returnInfo.type != entry.first)
				{
					errs() << "Found an external constructor \"" << method.sourceName << "\" whose return value doesn't match the external class, skipping.\n";
					continue;
				}
			}
			else
			{
				if (method.paramInfos.size() == 0)
				{
					errs() << "Found an external method \"" << method.sourceName << "\" with no parameters this isn't supported, skipping.\n";
					continue;
				}

				if (method.paramInfos[0].type != entry.first)
				{
					errs() << "Found an external method \"" << method.sourceName << "\" whose first parameter doesn't "
						" accept the class its operating on. This is not supported, skipping. \n";
					continue;
				}

				method.paramInfos.erase(method.paramInfos.begin());
			}

			classInfo->methodInfos.push_back(method);
		}
	}

	// Generate unique interop method names
	std::unordered_set<std::string> usedNames;
	for (auto& fileInfo : outputFileInfos)
	{
		for (auto& classInfo : fileInfo.second.classInfos)
		{
			usedNames.clear();

			auto generateInteropName = [&usedNames](MethodInfo& methodInfo)
			{
				std::string interopName = methodInfo.sourceName;
				int counter = 0;
				while (true)
				{
					auto iterFind = usedNames.find(interopName);
					if (iterFind == usedNames.end())
						break;

					interopName = methodInfo.sourceName + std::to_string(counter);
					counter++;
				}

				usedNames.insert(interopName);
				methodInfo.interopName = interopName;
			};

			for (auto& methodInfo : classInfo.methodInfos)
				generateInteropName(methodInfo);

			for (auto& methodInfo : classInfo.ctorInfos)
				generateInteropName(methodInfo);
		}
	}

	// Generate property infos
	for (auto& fileInfo : outputFileInfos)
	{
		for (auto& classInfo : fileInfo.second.classInfos)
		{
			for (auto& methodInfo : classInfo.methodInfos)
			{
				bool isGetter = (methodInfo.flags & (int)MethodFlags::PropertyGetter) != 0;
				bool isSetter = (methodInfo.flags & (int)MethodFlags::PropertySetter) != 0;

				if (!isGetter && !isSetter)
					continue;

				PropertyInfo propertyInfo;
				propertyInfo.name = methodInfo.scriptName;
				propertyInfo.documentation = methodInfo.documentation;
				propertyInfo.isStatic = (methodInfo.flags & (int)MethodFlags::Static);

				if (isGetter)
				{
					propertyInfo.getter = methodInfo.interopName;
					propertyInfo.type = methodInfo.returnInfo.type;
					propertyInfo.typeFlags = methodInfo.returnInfo.flags;
				}
				else // Setter
				{
					propertyInfo.setter = methodInfo.interopName;
					propertyInfo.type = methodInfo.paramInfos[0].type;
					propertyInfo.typeFlags = methodInfo.paramInfos[0].flags;
				}

				auto iterFind = std::find_if(classInfo.propertyInfos.begin(), classInfo.propertyInfos.end(),
					[&propertyInfo](const PropertyInfo& info)
				{
					return propertyInfo.name == info.name;
				});

				if (iterFind == classInfo.propertyInfos.end())
					classInfo.propertyInfos.push_back(propertyInfo);
				else
				{
					PropertyInfo& existingInfo = *iterFind;
					if (existingInfo.type != propertyInfo.type || existingInfo.isStatic != propertyInfo.isStatic)
					{
						errs() << "Getter and setter types for the property \"" << propertyInfo.name << "\" don't match. Skipping property.\n";
						continue;
					}

					if (!propertyInfo.getter.empty())
					{
						existingInfo.getter = propertyInfo.getter;

						// Prefer documentation from setter, but use getter if no other available
						if (existingInfo.documentation.empty())
							existingInfo.documentation = propertyInfo.documentation;
					}
					else
					{
						existingInfo.setter = propertyInfo.setter;

						if (!propertyInfo.documentation.empty())
							existingInfo.documentation = propertyInfo.documentation;
					}
				}
			}
		}
	}

	// Generate meta-data about base classes
	for (auto& fileInfo : outputFileInfos)
	{
		for (auto& classInfo : fileInfo.second.classInfos)
		{
			if (classInfo.baseClass.empty())
				continue;

			ClassInfo* baseClassInfo = findClassInfo(classInfo.baseClass);
			if (baseClassInfo == nullptr)
			{
				assert(false);
				continue;
			}

			baseClassInfo->flags |= (int)ClassFlags::IsBase;
		}
	}

	// Generate referenced includes
	{
		for (auto& fileInfo : outputFileInfos)
		{
			std::unordered_map<std::string, IncludeInfo> includes;
			for (auto& classInfo : fileInfo.second.classInfos)
				gatherIncludes(classInfo, includes);

			// Needed for all .h files
			if (!fileInfo.second.inEditor)
				fileInfo.second.referencedHeaderIncludes.push_back("BsScriptEnginePrerequisites.h");
			else
				fileInfo.second.referencedHeaderIncludes.push_back("BsScriptEditorPrerequisites.h");

			// Needed for all .cpp files
			fileInfo.second.referencedSourceIncludes.push_back("BsScript" + fileInfo.first + ".generated.h");
			fileInfo.second.referencedSourceIncludes.push_back("BsMonoClass.h");
			fileInfo.second.referencedSourceIncludes.push_back("BsMonoUtil.h");

			for (auto& classInfo : fileInfo.second.classInfos)
			{
				UserTypeInfo& typeInfo = cppToCsTypeMap[classInfo.name];

				if (typeInfo.type == ParsedType::Resource)
					fileInfo.second.referencedHeaderIncludes.push_back("BsScriptResource.h");
				else if (typeInfo.type == ParsedType::Component)
					fileInfo.second.referencedHeaderIncludes.push_back("BsScriptComponent.h");
				else // Class
					fileInfo.second.referencedHeaderIncludes.push_back("BsScriptObject.h");

				if (!classInfo.baseClass.empty())
				{
					UserTypeInfo& baseTypeInfo = cppToCsTypeMap[classInfo.baseClass];

					std::string include = "BsScript" + baseTypeInfo.destFile + ".generated.h";
					fileInfo.second.referencedHeaderIncludes.push_back(include);
				}

				fileInfo.second.referencedSourceIncludes.push_back(typeInfo.declFile);
			}

			for (auto& structInfo : fileInfo.second.structInfos)
			{
				UserTypeInfo& typeInfo = cppToCsTypeMap[structInfo.name];

				fileInfo.second.referencedHeaderIncludes.push_back("BsScriptObject.h");
				fileInfo.second.referencedHeaderIncludes.push_back(typeInfo.declFile);
			}

			for (auto& entry : includes)
			{
				if (entry.second.sourceInclude)
				{
					std::string include = entry.second.typeInfo.declFile;
					fileInfo.second.referencedHeaderIncludes.push_back(include);
				}

				if (entry.second.typeInfo.type != ParsedType::Enum)
				{
					if (!entry.second.typeInfo.destFile.empty())
					{
						std::string include = "BsScript" + entry.second.typeInfo.destFile + ".generated.h";
						fileInfo.second.referencedSourceIncludes.push_back(include);
					}
				}
			}
		}
	}
}

std::string generateCppMethodSignature(const MethodInfo& methodInfo, const std::string& thisPtrType, const std::string& nestedName)
{
	bool isStatic = (methodInfo.flags & (int)MethodFlags::Static) != 0;
	bool isCtor = (methodInfo.flags & (int)MethodFlags::Constructor) != 0;

	std::stringstream output;

	bool returnAsParameter = false;
	if (methodInfo.returnInfo.type.empty() || isCtor)
		output << "void";
	else
	{
		UserTypeInfo returnTypeInfo = getTypeInfo(methodInfo.returnInfo.type, methodInfo.returnInfo.flags);
		if (!canBeReturned(returnTypeInfo.type, methodInfo.returnInfo.flags))
		{
			output << "void";
			returnAsParameter = true;
		}
		else
		{
			output << getInteropCppVarType(methodInfo.returnInfo.type, returnTypeInfo.type, methodInfo.returnInfo.flags);
		}
	}

	output << " ";

	if (!nestedName.empty())
		output << nestedName << "::";

	output << "Internal_" << methodInfo.interopName << "(";

	if (isCtor)
	{
		output << "MonoObject* managedInstance";

		if (methodInfo.paramInfos.size() > 0)
			output << ", ";
	}
	else if (!isStatic)
	{
		output << thisPtrType << "* thisPtr";

		if (methodInfo.paramInfos.size() > 0 || returnAsParameter)
			output << ", ";
	}

	for (auto I = methodInfo.paramInfos.begin(); I != methodInfo.paramInfos.end(); ++I)
	{
		UserTypeInfo paramTypeInfo = getTypeInfo(I->type, I->flags);

		output << getInteropCppVarType(I->type, paramTypeInfo.type, I->flags) << " " << I->name;

		if ((I + 1) != methodInfo.paramInfos.end() || returnAsParameter)
			output << ", ";
	}

	if (returnAsParameter)
	{
		UserTypeInfo returnTypeInfo = getTypeInfo(methodInfo.returnInfo.type, methodInfo.returnInfo.flags);

		output << getInteropCppVarType(methodInfo.returnInfo.type, returnTypeInfo.type, methodInfo.returnInfo.flags) <<
			" " << "__output";
	}

	output << ")";
	return output.str();
}

std::string generateNativeToScriptObjectLine(ParsedType type, const std::string& scriptType, const std::string& scriptName,
	const std::string& argName, const std::string& indent = "\t\t")
{
	std::stringstream output;

	output << indent << scriptType << "* " << scriptName << ";" << std::endl;

	if (type == ParsedType::Resource)
	{
		output << indent << "ScriptResourceManager::instance().getScriptResource(" << argName << ", &" <<
			scriptName << ", true);" << std::endl;
	}
	else if (type == ParsedType::Component)
	{
		output << indent << scriptName << " = ScriptGameObjectManager::instance().getBuiltinScriptComponent(" <<
			argName << ");" << std::endl;
	}
	else if (type == ParsedType::SceneObject)
	{
		output << indent << scriptName << " = ScriptGameObjectManager::instance().getOrCreateScriptSceneObject(" <<
			argName << ");" << std::endl;
	}
	else if (type == ParsedType::Class)
	{
		output << indent << scriptName << " = " << scriptType << "::create(" << argName << ");" << std::endl;
	}
	else
		assert(false);

	return output.str();
}

std::string generateMethodBodyBlockForParam(const std::string& name, const std::string& typeName, int flags,
	bool isLast, bool returnValue, std::stringstream& preCallActions, std::stringstream& postCallActions)
{
	UserTypeInfo paramTypeInfo = getTypeInfo(typeName, flags);

	if (!isArray(flags))
	{
		std::string argName;

		switch (paramTypeInfo.type)
		{
		case ParsedType::Builtin:
		case ParsedType::Enum:
		case ParsedType::Struct:
			if (returnValue)
			{
				argName = "tmp" + name;
				preCallActions << "\t\t" << typeName << " " << argName << ";" << std::endl;

				if (paramTypeInfo.type == ParsedType::Struct)
					postCallActions << "\t\t*" << name << " = " << argName << ";" << std::endl;
				else
					postCallActions << "\t\t" << name << " = " << argName << ";" << std::endl;
			}
			else if (isOutput(flags))
			{
				argName = "tmp" + name;
				preCallActions << "\t\t" << typeName << " " << argName << ";" << std::endl;
				postCallActions << "\t\t*" << name << " = " << argName << ";" << std::endl;
			}
			else
				argName = name;

			break;
		case ParsedType::String:
		{
			argName = "tmp" + name;
			preCallActions << "\t\tString " << argName << ";" << std::endl;

			if (returnValue)
				postCallActions << "\t\t" << name << " = MonoUtil::stringToMono(" << argName << ");" << std::endl;
			else if (isOutput(flags))
				postCallActions << "\t\t*" << name << " = MonoUtil::stringToMono(" << argName << ");" << std::endl;
			else
				preCallActions << "\t\t" << argName << " = MonoUtil::monoToString(" << name << ");" << std::endl;
		}
		break;
		case ParsedType::WString:
		{
			argName = "tmp" + name;
			preCallActions << "\t\tWString " << argName << ";" << std::endl;

			if (returnValue)
				postCallActions << "\t\t" << name << " = MonoUtil::wstringToMono(" << argName << ");" << std::endl;
			else if (isOutput(flags))
				postCallActions << "\t\t*" << name << " = MonoUtil::wstringToMono(" << argName << ");" << std::endl;
			else
				preCallActions << "\t\t" << argName << " = MonoUtil::monoToWString(" << name << ");" << std::endl;
		}
		break;
		default: // Some object type
		{
			argName = "tmp" + name;
			std::string tmpType = getCppVarType(typeName, paramTypeInfo.type);

			preCallActions << "\t\t" << tmpType << " " << argName << ";" << std::endl;

			std::string scriptName = "script" + name;
			std::string scriptType = getScriptInteropType(typeName);

			if (returnValue)
			{
				postCallActions << generateNativeToScriptObjectLine(paramTypeInfo.type, scriptType, scriptName, argName);
				postCallActions << "\t\t" << name << " = " << scriptName << "->getMangedInstance();" << std::endl;
			}
			else if (isOutput(flags))
			{
				postCallActions << generateNativeToScriptObjectLine(paramTypeInfo.type, scriptType, scriptName, argName);
				postCallActions << "\t\t*" << name << " = " << scriptName << "->getMangedInstance();" << std::endl;
			}
			else
			{
				preCallActions << "\t\t" << scriptType << "* " << scriptName << ";" << std::endl;
				preCallActions << "\t\t" << scriptName << " = " << scriptType << "::toNative(" << name << ");" << std::endl;

				if (isHandleType(paramTypeInfo.type))
					preCallActions << "\t\t" << argName << " = " << scriptName << "->getHandle();" << std::endl;
				else
					preCallActions << "\t\t" << argName << " = " << scriptName << "->getInternal();" << std::endl;
			}
		}
		break;
		}

		return argName;
	}
	else
	{
		std::string entryType;
		switch (paramTypeInfo.type)
		{
		case ParsedType::Builtin:
		case ParsedType::String:
		case ParsedType::WString:
		case ParsedType::Enum:
			entryType = typeName;
			break;
		default: // Some object or struct type
			entryType = getScriptInteropType(typeName);
			break;
		}

		std::string argType = "Vector<" + typeName + ">";
		std::string argName = "vec" + name;

		if (!isOutput(flags) && !returnValue)
		{
			std::string arrayName = "array" + name;
			preCallActions << "\t\tScriptArray " << arrayName << "(" << name << ");" << std::endl;
			preCallActions << "\t\t" << argType << " " << argName << "(" << arrayName << ".size());" << std::endl;

			preCallActions << "\t\tfor(int i = 0; i < " << arrayName << ".size(); i++)" << std::endl;
			preCallActions << "\t\t{" << std::endl;

			switch (paramTypeInfo.type)
			{
			case ParsedType::Builtin:
			case ParsedType::String:
			case ParsedType::WString:
				preCallActions << "\t\t\t" << argName << "[i] = " << arrayName << ".get<" << entryType << ">(i);" << std::endl;
				break;
			case ParsedType::Enum:
			{
				std::string enumType;
				mapBuiltinTypeToCppType(paramTypeInfo.underlyingType, enumType);

				preCallActions << "\t\t\t" << argName << "[i] = (" << entryType << ")" << arrayName << ".get<" << enumType << ">(i);" << std::endl;
				break;
			}
			case ParsedType::Struct:
				preCallActions << "\t\t\t" << argName << "[i] = " << entryType << "::unbox(" << arrayName << ".get<MonoObject*>(i));" << std::endl;
				break;
			default: // Some object type
			{
				std::string scriptName = "script" + name;
				preCallActions << "\t\t\t" << entryType << "* " << scriptName << ";" << std::endl;
				preCallActions << "\t\t\t" << scriptName << " = " << entryType << "::toNative(" << arrayName << ".get<MonoObject*>(i));" << std::endl;
				preCallActions << "\t\t\tif(scriptName != nullptr)" << std::endl;

				if (isHandleType(paramTypeInfo.type))
					preCallActions << "\t\t\t\t" << argName << "[i] = " << scriptName << "->getHandle();" << std::endl;
				else
					preCallActions << "\t\t\t\t" << argName << "[i] = " << scriptName << "->getInternal();" << std::endl;
			}
			break;
			}

			preCallActions << "\t\t}" << std::endl;

			if (!isLast)
				preCallActions << std::endl;
		}
		else
		{
			preCallActions << "\t\t" << argType << " " << argName << ";" << std::endl;

			std::string arrayName = "array" + name;
			postCallActions << "\t\tScriptArray " << arrayName << ";" << std::endl;
			postCallActions << "\t\t" << arrayName << " = " << "ScriptArray::create<" << entryType << ">((int)" << argName << ".size());" << std::endl;
			postCallActions << "\t\tfor(int i = 0; i < (int)" << argName << ".size(); i++)" << std::endl;
			postCallActions << "\t\t{" << std::endl;

			switch (paramTypeInfo.type)
			{
			case ParsedType::Builtin:
			case ParsedType::String:
			case ParsedType::WString:
				postCallActions << "\t\t\t" << arrayName << ".set(i, " << argName << "[i]);" << std::endl;
				break;
			case ParsedType::Enum:
			{
				std::string enumType;
				mapBuiltinTypeToCppType(paramTypeInfo.underlyingType, enumType);

				postCallActions << "\t\t\t" << arrayName << ".set(i, (" << enumType << ")" << argName << "[i]);" << std::endl;
				break;
			}
			case ParsedType::Struct:
				postCallActions << "\t\t\t" << arrayName << ".set(i, " << entryType << "::box(" << argName << "[i]));" << std::endl;
				break;
			default: // Some object type
			{
				std::string scriptName = "script" + name;

				postCallActions << generateNativeToScriptObjectLine(paramTypeInfo.type, entryType, scriptName, argName + "[i]", "\t\t\t");
				postCallActions << "\t\t\t" << arrayName << ".set(i, " << scriptName << "->getMangedInstance());" << std::endl;
			}
			break;
			}

			postCallActions << "\t\t}" << std::endl;

			if (returnValue)
				postCallActions << "\t\t" << name << " = " << arrayName << ".getInternal()" << std::endl;
			else
				postCallActions << "\t\t*" << name << " = " << arrayName << ".getInternal()" << std::endl;
		}

		return argName;
	}
}

std::string generateCppMethodBody(const MethodInfo& methodInfo, const std::string& sourceClassName,
	const std::string& interopClassName, ParsedType classType)
{
	std::string returnAssignment;
	std::string returnStmt;
	std::stringstream preCallActions;
	std::stringstream methodArgs;
	std::stringstream postCallActions;

	bool isStatic = (methodInfo.flags & (int)MethodFlags::Static) != 0;
	bool isCtor = (methodInfo.flags & (int)MethodFlags::Constructor) != 0;
	bool isExternal = (methodInfo.flags & (int)MethodFlags::External) != 0;

	bool returnAsParameter = false;
	UserTypeInfo returnTypeInfo;
	if (!methodInfo.returnInfo.type.empty() && !isCtor)
	{
		returnTypeInfo = getTypeInfo(methodInfo.returnInfo.type, methodInfo.returnInfo.flags);
		if (!canBeReturned(returnTypeInfo.type, methodInfo.returnInfo.flags))
			returnAsParameter = true;
		else
		{
			std::string returnType = getInteropCppVarType(methodInfo.returnInfo.type, returnTypeInfo.type, methodInfo.returnInfo.flags);
			postCallActions << "\t\t" << returnType << " __output;" << std::endl;

			std::string argName = generateMethodBodyBlockForParam("__output", methodInfo.returnInfo.type,
				methodInfo.returnInfo.flags, true, true, preCallActions, postCallActions);

			returnAssignment = argName + " = ";
			returnStmt = "\t\treturn __output;";
		}
	}

	for (auto I = methodInfo.paramInfos.begin(); I != methodInfo.paramInfos.end(); ++I)
	{
		bool isLast = (I + 1) == methodInfo.paramInfos.end();

		std::string argName = generateMethodBodyBlockForParam(I->name, I->type, I->flags, isLast, false,
			preCallActions, postCallActions);

		if (!isArray(I->flags))
		{
			UserTypeInfo paramTypeInfo = getTypeInfo(I->type, I->flags);

			methodArgs << getAsCppArgument(argName, paramTypeInfo.type, I->flags, methodInfo.sourceName);
		}
		else
			methodArgs << getAsCppArgument(argName, ParsedType::Builtin, I->flags, methodInfo.sourceName);

		if (!isLast)
			methodArgs << ", ";
	}

	if (returnAsParameter)
	{
		std::string argName = generateMethodBodyBlockForParam("__output", methodInfo.returnInfo.type,
			methodInfo.returnInfo.flags, true, true, preCallActions, postCallActions);

		returnAssignment = argName + " = ";
	}

	std::stringstream output;
	output << "\t{" << std::endl;
	output << preCallActions.str();

	if (isCtor)
	{
		bool isValid = false;
		if (!isExternal)
		{
			if (classType == ParsedType::Class)
			{
				output << "\t\tSPtr<" << sourceClassName << "> instance = bs_shared_ptr_new<" << sourceClassName << ">(" << methodArgs.str() << ");" << std::endl;
				isValid = true;
			}
		}
		else
		{
			std::string fullMethodName = methodInfo.externalClass + "::" + methodInfo.sourceName;

			if (classType == ParsedType::Class)
			{
				output << "\t\tSPtr<" << sourceClassName << "> instance = " << fullMethodName << "(" << methodArgs.str() << ");" << std::endl;
				isValid = true;
			}
			else if (classType == ParsedType::Resource)
			{
				output << "\t\tResourceHandle<" << sourceClassName << "> instance = " << fullMethodName << "(" << methodArgs.str() << ");" << std::endl;
				isValid = true;
			}
		}

		if (isValid)
			output << "\t\t" << interopClassName << "* scriptInstance = new (bs_alloc<" << interopClassName << ">())" << interopClassName << "(managedInstance, instance);" << std::endl;
		else
			errs() << "Cannot generate a constructor for \"" << sourceClassName << "\". Unsupported class type. \n";
	}
	else
	{
		if (!isExternal)
		{
			if (isStatic)
			{
				output << "\t\t" << returnAssignment << sourceClassName << "::" << methodInfo.sourceName << "(" << methodArgs.str() << ");" << std::endl;
			}
			else
			{
				if (classType == ParsedType::Class)
				{
					output << "\t\t" << returnAssignment << "thisPtr->getInternal()->" << methodInfo.sourceName << "(" << methodArgs.str() << ");" << std::endl;
				}
				else // Must be one of the handle types
				{
					assert(isHandleType(classType));

					output << "\t\t" << returnAssignment << "thisPtr->getHandle()->" << methodInfo.sourceName << "(" << methodArgs.str() << ");" << std::endl;
				}
			}
		}
		else
		{
			std::string fullMethodName = methodInfo.externalClass + "::" + methodInfo.sourceName;
			if (isStatic)
			{
				output << "\t\t" << returnAssignment << fullMethodName << "(" << methodArgs.str() << ");" << std::endl;
			}
			else
			{
				if (classType == ParsedType::Class)
				{
					output << "\t\t" << returnAssignment << fullMethodName << "(thisPtr->getInternal()";
				}
				else // Must be one of the handle types
				{
					assert(isHandleType(classType));

					output << "\t\t" << returnAssignment << fullMethodName << "(thisPtr->getHandle()";
				}

				std::string methodArgsStr = methodArgs.str();
				if (!methodArgsStr.empty())
					output << ", " << methodArgsStr;

				output << ");" << std::endl;
			}
		}
	}

	std::string postCallActionsStr = postCallActions.str();
	if (!postCallActionsStr.empty())
		output << std::endl;

	output << postCallActionsStr;

	if (!returnStmt.empty())
	{
		output << std::endl;
		output << returnStmt << std::endl;
	}

	output << "\t}" << std::endl;
	return output.str();
}

std::string generateCppHeaderOutput(const ClassInfo& classInfo, const UserTypeInfo& typeInfo)
{
	bool inEditor = (classInfo.flags & (int)ClassFlags::Editor) != 0;
	bool isBase = (classInfo.flags & (int)ClassFlags::IsBase) != 0;
	bool isRootBase = classInfo.baseClass.empty();

	std::string exportAttr;
	if (!inEditor)
		exportAttr = "BS_SCR_BE_EXPORT";
	else
		exportAttr = "BS_SCR_BED_EXPORT";

	std::string wrappedDataType = getCppVarType(classInfo.name, typeInfo.type);
	std::string interopBaseClassName;

	std::stringstream output;

	// Generate base class if required
	if (isBase)
	{
		interopBaseClassName = getScriptInteropType(classInfo.name) + "Base";

		output << "\tclass " << exportAttr << " ";
		output << interopBaseClassName << " : public ";

		if (isRootBase)
		{
			if (typeInfo.type == ParsedType::Class)
				output << "ScriptObjectBase";
			else if (typeInfo.type == ParsedType::Component)
				output << "ScriptComponentBase";
			else if (typeInfo.type == ParsedType::Resource)
				output << "ScriptResourceBase";
		}
		else
		{
			std::string parentBaseClassName = getScriptInteropType(classInfo.baseClass) + "Base";
			output << parentBaseClassName;
		}

		output << std::endl;
		output << "\t{" << std::endl;
		output << "\tpublic:" << std::endl;
		output << interopBaseClassName << "(MonoObject* instance);" << std::endl;
		output << "virtual ~" << interopBaseClassName << "() {}" << std::endl;

		if (typeInfo.type == ParsedType::Class)
		{
			output << std::endl;
			output << "\t\t" << wrappedDataType << " getInternal() const { return mInternal; }" << std::endl;

			// Data member only present in the top-most base class
			if (isRootBase)
			{
				output << "\tprotected:" << std::endl;
				output << "\t\t" << wrappedDataType << " mInternal;" << std::endl;
			}
		}

		output << "\t};" << std::endl;
		output << std::endl;
	}
	else if (!classInfo.baseClass.empty())
	{
		interopBaseClassName = getScriptInteropType(classInfo.baseClass) + "Base";
	}

	// Generate main class
	output << "\tclass " << exportAttr << " ";;

	std::string interopClassName = getScriptInteropType(classInfo.name);
	output << interopClassName << " : public ";

	if (typeInfo.type == ParsedType::Resource)
		output << "TScriptResource<" << interopClassName << ", " << classInfo.name;
	else if (typeInfo.type == ParsedType::Component)
		output << "TScriptComponent<" << interopClassName << ", " << classInfo.name;
	else // Class
		output << "ScriptObject<" << interopClassName;

	if (!interopBaseClassName.empty())
		output << ", " << interopBaseClassName;

	output << ">";

	output << std::endl;
	output << "\t{" << std::endl;
	output << "\tpublic:" << std::endl;

	if (!inEditor)
		output << "\t\tSCRIPT_OBJ(ENGINE_ASSEMBLY, \"BansheeEngine\", \"" << typeInfo.scriptName << "\")" << std::endl;
	else
		output << "\t\tSCRIPT_OBJ(EDITOR_ASSEMBLY, \"BansheeEditor\", \"" << typeInfo.scriptName << "\")" << std::endl;

	output << std::endl;

	if (typeInfo.type == ParsedType::Class)
	{
		// getInternal() method (handle types have getHandle() implemented by their base type)
		output << "\t\t" << wrappedDataType << " getInternal() const { return mInternal; }" << std::endl;

		// create() method
		output << "\t\tstatic MonoObject* create(const " << wrappedDataType << "& value);" << std::endl;
	}
	else if (typeInfo.type == ParsedType::Resource)
	{
		// createInstance() method required by script resource manager
		output << "\t\tstatic MonoObject* createInstance();" << std::endl;
	}

	output << std::endl;
	output << "\tprivate:" << std::endl;

	// Constructor
	output << "\t\t" << interopClassName << "(MonoObject* managedInstance, const " << wrappedDataType << "& value);" << std::endl;
	output << std::endl;

	// Data member
	if (typeInfo.type == ParsedType::Class)
	{
		output << "\t\t" << wrappedDataType << " mInternal;" << std::endl;
		output << std::endl;
	}

	// CLR hooks
	std::string interopClassThisPtrType;
	if (isBase)
		interopClassThisPtrType = interopBaseClassName;
	else
		interopClassThisPtrType = interopClassName;

	for (auto& methodInfo : classInfo.ctorInfos)
		output << "\t\tstatic " << generateCppMethodSignature(methodInfo, interopClassThisPtrType, "") << ";" << std::endl;

	for (auto& methodInfo : classInfo.methodInfos)
		output << "\t\tstatic " << generateCppMethodSignature(methodInfo, interopClassThisPtrType, "") << ";" << std::endl;

	output << "\t};" << std::endl;
	return output.str();
}

std::string generateCppSourceOutput(const ClassInfo& classInfo, const UserTypeInfo& typeInfo)
{
	bool isBase = (classInfo.flags & (int)ClassFlags::IsBase) != 0;

	std::string interopClassName = getScriptInteropType(classInfo.name);
	std::string wrappedDataType = getCppVarType(classInfo.name, typeInfo.type);

	std::string interopBaseClassName;
	if (isBase)
		interopBaseClassName = getScriptInteropType(classInfo.name) + "Base";
	else if (!classInfo.baseClass.empty())
		interopBaseClassName = getScriptInteropType(classInfo.baseClass) + "Base";

	std::stringstream output;

	// Constructor
	output << "\t" << interopClassName << "::" << interopClassName << "(MonoObject* managedInstance, const " << wrappedDataType << "& value)" << std::endl;
	output << "\t\t:";

	if (typeInfo.type == ParsedType::Resource)
		output << "TScriptResource(managedInstance, value)";
	else if (typeInfo.type == ParsedType::Component)
		output << "TScriptComponent(managedInstance, value)";
	else // Class
		output << "ScriptObject(managedInstance), mInternal(value)";

	output << std::endl;
	output << "\t{ }" << std::endl;
	output << std::endl;

	// CLR hook registration
	output << "\tvoid " << interopClassName << "::initRuntimeData()" << std::endl;
	output << "\t{" << std::endl;

	for (auto& methodInfo : classInfo.ctorInfos)
	{
		output << "\t\tmetaData.scriptClass->addInternalCall(\"Internal_" << methodInfo.interopName << "\", &" <<
			interopClassName << "::Internal_" << methodInfo.interopName << ");" << std::endl;
	}

	for (auto& methodInfo : classInfo.methodInfos)
	{
		output << "\t\tmetaData.scriptClass->addInternalCall(\"Internal_" << methodInfo.interopName << "\", &" <<
			interopClassName << "::Internal_" << methodInfo.interopName << ");" << std::endl;
	}

	output << "\t}" << std::endl;
	output << std::endl;

	// create() or createInstance() methods
	if (typeInfo.type == ParsedType::Class || typeInfo.type == ParsedType::Resource)
	{
		std::stringstream ctorSignature;
		std::stringstream ctorParamsInit;
		MethodInfo unusedCtor = findUnusedCtorSignature(classInfo);
		int numDummyParams = (int)unusedCtor.paramInfos.size();

		ctorParamsInit << "\t\tbool dummy = false;" << std::endl;
		ctorParamsInit << "\t\tvoid* ctorParams[" << numDummyParams << "] = { ";

		for (int i = 0; i < numDummyParams; i++)
		{
			ctorParamsInit << "&dummy";
			ctorSignature << unusedCtor.paramInfos[i].type;

			if ((i + 1) < numDummyParams)
			{
				ctorParamsInit << ", ";
				ctorSignature << ",";
			}
		}

		ctorParamsInit << " };" << std::endl;
		ctorParamsInit << std::endl;

		if (typeInfo.type == ParsedType::Class)
		{
			output << "\tMonoObject* " << interopClassName << "::create(const " << wrappedDataType << "& value)" << std::endl;
			output << "\t{" << std::endl;

			output << ctorParamsInit.str();
			output << "\t\tMonoObject* managedInstance = metaData.scriptClass->createInstance(" << ctorSignature.str() << ", ctorParams);" << std::endl;
			output << "\t\t" << interopClassName << "* scriptInstance = new (bs_alloc<" << interopClassName << ">()) " << interopClassName << "(managedInstance, value);" << std::endl;
			output << "\t\treturn managedInstance;" << std::endl;

			output << "\t}" << std::endl;
		}
		else if (typeInfo.type == ParsedType::Resource)
		{
			output << "\t MonoObject*" << interopClassName << "::createInstance()" << std::endl;
			output << "\t{" << std::endl;

			output << ctorParamsInit.str();
			output << "\t\treturn metaData.scriptClass->createInstance(" << ctorSignature.str() << ", ctorParams);" << std::endl;

			output << "\t}" << std::endl;
		}
	}

	// CLR hook method implementations
	std::string interopClassThisPtrType;
	if (isBase)
		interopClassThisPtrType = interopBaseClassName;
	else
		interopClassThisPtrType = interopClassName;

	for (auto I = classInfo.ctorInfos.begin(); I != classInfo.ctorInfos.end(); ++I)
	{
		const MethodInfo& methodInfo = *I;

		output << "\t" << generateCppMethodSignature(methodInfo, interopClassThisPtrType, interopClassName) << std::endl;
		output << generateCppMethodBody(methodInfo, classInfo.name, interopClassName, typeInfo.type);

		if ((I + 1) != classInfo.methodInfos.end())
			output << std::endl;
	}

	for (auto I = classInfo.methodInfos.begin(); I != classInfo.methodInfos.end(); ++I)
	{
		const MethodInfo& methodInfo = *I;

		output << "\t" << generateCppMethodSignature(methodInfo, interopClassThisPtrType, interopClassName) << std::endl;
		output << generateCppMethodBody(methodInfo, classInfo.name, interopClassName, typeInfo.type);

		if ((I + 1) != classInfo.methodInfos.end())
			output << std::endl;
	}

	return output.str();
}

std::string generateCppStructHeader(const StructInfo& structInfo)
{
	UserTypeInfo typeInfo = getTypeInfo(structInfo.name, 0);

	std::stringstream output;

	output << "\tclass ";

	if (!structInfo.inEditor)
		output << "BS_SCR_BE_EXPORT ";
	else
		output << "BS_SCR_BED_EXPORT ";

	std::string interopClassName = getScriptInteropType(structInfo.name);
	output << interopClassName << " : public " << "ScriptObject<" << interopClassName << ">";

	output << std::endl;
	output << "\t{" << std::endl;
	output << "\tpublic:" << std::endl;

	if (!structInfo.inEditor)
		output << "\t\tSCRIPT_OBJ(ENGINE_ASSEMBLY, \"BansheeEngine\", \"" << typeInfo.scriptName << "\")" << std::endl;
	else
		output << "\t\tSCRIPT_OBJ(EDITOR_ASSEMBLY, \"BansheeEditor\", \"" << typeInfo.scriptName << "\")" << std::endl;

	output << std::endl;

	output << "\t\tstatic MonoObject* box(const " << structInfo.name << "& value);" << std::endl;
	output << "\t\tstatic " << structInfo.name << " unbox(MonoObject* value);" << std::endl;

	output << std::endl;
	output << "\tprivate:" << std::endl;

	// Constructor
	output << "\t\t" << interopClassName << "(MonoObject* managedInstance);" << std::endl;
	output << std::endl;

	output << "\t};" << std::endl;
	return output.str();
}

std::string generateCppStructSource(const StructInfo& structInfo)
{
	UserTypeInfo typeInfo = getTypeInfo(structInfo.name, 0);
	std::string interopClassName = getScriptInteropType(structInfo.name);

	std::stringstream output;

	// Constructor
	output << "\t" << interopClassName << "::" << interopClassName << "(MonoObject* managedInstance)" << std::endl;
	output << "\t\t:ScriptObject(managedInstance)" << std::endl;
	output << "\t{ }" << std::endl;
	output << std::endl;

	// Empty initRuntimeData
	output << "\tvoid " << interopClassName << "::initRuntimeData()" << std::endl;
	output << "\t{ }" << std::endl;
	output << std::endl;

	// Box
	output << "\tMonoObject*" << interopClassName << "::box(const " << structInfo.name << "& value)" << std::endl;
	output << "\t{" << std::endl;
	output << "\t\treturn MonoUtil::box(metaData.scriptClass->_getInternalClass(), (void*)&value);" << std::endl;
	output << "\t}" << std::endl;
	output << std::endl;

	// Unbox
	output << "\t" << structInfo.name << " " << interopClassName << "::unbox(MonoObject* value)" << std::endl;
	output << "\t{" << std::endl;
	output << "\t\treturn *(" << structInfo.name << "*)MonoUtil::unbox(value);" << std::endl;
	output << "\t}" << std::endl;
	output << std::endl;

	return output.str();
}

std::string generateCSMethodParams(const MethodInfo& methodInfo, bool forInterop)
{
	std::stringstream output;
	for (auto I = methodInfo.paramInfos.begin(); I != methodInfo.paramInfos.end(); ++I)
	{
		const VarInfo& paramInfo = *I;
		UserTypeInfo paramTypeInfo = getTypeInfo(paramInfo.type, paramInfo.flags);
		std::string qualifiedType = getCSVarType(paramTypeInfo.scriptName, paramTypeInfo.type, paramInfo.flags, true, true, forInterop);

		output << qualifiedType << " " << paramInfo.name;

		if (!forInterop && !paramInfo.defaultValue.empty())
			output << " = " << paramInfo.defaultValue;

		if ((I + 1) != methodInfo.paramInfos.end())
			output << ", ";
	}

	return output.str();
}

std::string generateCSMethodArgs(const MethodInfo& methodInfo, bool forInterop)
{
	std::stringstream output;
	for (auto I = methodInfo.paramInfos.begin(); I != methodInfo.paramInfos.end(); ++I)
	{
		const VarInfo& paramInfo = *I;
		UserTypeInfo paramTypeInfo = getTypeInfo(paramInfo.type, paramInfo.flags);

		if (isOutput(paramInfo.flags))
		{
			if (isPlainStruct(paramTypeInfo.type, paramInfo.flags))
				output << "ref ";
			else
				output << "out ";
		}
		else if (forInterop && isPlainStruct(paramTypeInfo.type, paramInfo.flags))
			output << "ref ";

		output << paramInfo.name;

		if ((I + 1) != methodInfo.paramInfos.end())
			output << ", ";
	}

	return output.str();
}

std::string generateCSInteropMethodSignature(const MethodInfo& methodInfo, const std::string& csClassName)
{
	bool isStatic = (methodInfo.flags & (int)MethodFlags::Static) != 0;
	bool isCtor = (methodInfo.flags & (int)MethodFlags::Constructor) != 0;

	std::stringstream output;

	bool returnAsParameter = false;
	if (methodInfo.returnInfo.type.empty() || isCtor)
		output << "void";
	else
	{
		UserTypeInfo returnTypeInfo = getTypeInfo(methodInfo.returnInfo.type, methodInfo.returnInfo.flags);
		if (!canBeReturned(returnTypeInfo.type, methodInfo.returnInfo.flags))
		{
			output << "void";
			returnAsParameter = true;
		}
		else
		{
			std::string qualifiedType = getCSVarType(returnTypeInfo.scriptName, returnTypeInfo.type,
				methodInfo.returnInfo.flags, false, true, false);
			output << qualifiedType;
		}
	}

	output << " ";

	output << "Internal_" << methodInfo.interopName << "(";

	if (isCtor)
	{
		output << csClassName << " managedInstance";

		if (methodInfo.paramInfos.size() > 0)
			output << ", ";
	}
	else if (!isStatic)
	{
		output << "IntPtr thisPtr";

		if (methodInfo.paramInfos.size() > 0 || returnAsParameter)
			output << ", ";
	}

	output << generateCSMethodParams(methodInfo, true);

	if (returnAsParameter)
	{
		UserTypeInfo returnTypeInfo = getTypeInfo(methodInfo.returnInfo.type, methodInfo.returnInfo.flags);
		std::string qualifiedType = getCSVarType(returnTypeInfo.scriptName, returnTypeInfo.type, methodInfo.returnInfo.flags, true, true, true);

		if (methodInfo.paramInfos.size() > 0)
			output << ", ";

		output << qualifiedType << " __output";
	}

	output << ")";
	return output.str();
}

std::string generateCSClass(ClassInfo& input, UserTypeInfo& typeInfo)
{
	std::stringstream ctors;
	std::stringstream properties;
	std::stringstream methods;
	std::stringstream interops;

	// Private constructor for runtime use
	MethodInfo pvtCtor = findUnusedCtorSignature(input);

	ctors << "\t\tprivate " << typeInfo.scriptName << "(" << generateCSMethodParams(pvtCtor, false) << ") { }" << std::endl;
	ctors << std::endl;

	// Constructors
	for (auto& entry : input.ctorInfos)
	{
		// Generate interop
		interops << "\t\t[MethodImpl(MethodImplOptions.InternalCall)]" << std::endl;
		interops << "\t\tprivate static extern void Internal_" << entry.interopName << "(" << typeInfo.scriptName
			<< " managedInstance, " << generateCSMethodParams(entry, true) << ");";
		interops << std::endl;

		bool interopOnly = (entry.flags & (int)MethodFlags::InteropOnly) != 0;
		if (interopOnly)
			continue;

		ctors << entry.documentation;
		ctors << "\t\tpublic " << typeInfo.scriptName << "(" << generateCSMethodParams(entry, false) << ")" << std::endl;
		ctors << "\t\t{" << std::endl;
		ctors << "\t\t\tInternal_" << entry.interopName << "(this";

		if (entry.paramInfos.size() > 0)
			ctors << ", " << generateCSMethodArgs(entry, true);

		ctors << ");" << std::endl;
		ctors << "\t\t}" << std::endl;
		ctors << std::endl;
	}

	// External constructors, methods and interop stubs
	for (auto& entry : input.methodInfos)
	{
		// Generate interop
		interops << "\t\t[MethodImpl(MethodImplOptions.InternalCall)]" << std::endl;
		interops << "\t\tprivate static extern " << generateCSInteropMethodSignature(entry, typeInfo.scriptName) << ";";
		interops << std::endl;

		bool interopOnly = (entry.flags & (int)MethodFlags::InteropOnly) != 0;
		if (interopOnly)
			continue;

		bool isConstructor = (entry.flags & (int)MethodFlags::Constructor) != 0;
		bool isStatic = (entry.flags & (int)MethodFlags::Static) != 0;

		if (isConstructor)
		{
			ctors << entry.documentation;
			ctors << "\t\tpublic " << typeInfo.scriptName << "(" << generateCSMethodParams(entry, false) << ")" << std::endl;
			ctors << "\t\t{" << std::endl;
			ctors << "\t\t\tInternal_" << entry.interopName << "(this";

			if (entry.paramInfos.size() > 0)
				ctors << ", " << generateCSMethodArgs(entry, true);

			ctors << ");" << std::endl;
			ctors << "\t\t}" << std::endl;
			ctors << std::endl;
		}
		else
		{
			bool isProperty = entry.flags & ((int)MethodFlags::PropertyGetter | (int)MethodFlags::PropertySetter);
			if (!isProperty)
			{
				UserTypeInfo returnTypeInfo;
				std::string returnType;
				if (entry.returnInfo.type.empty())
					returnType = "void";
				else
				{
					returnTypeInfo = getTypeInfo(entry.returnInfo.type, entry.returnInfo.flags);
					returnType = getCSVarType(returnTypeInfo.scriptName, returnTypeInfo.type, entry.returnInfo.flags, false, true, false);
				}

				methods << entry.documentation;
				methods << "\t\tpublic " << returnType << " " << entry.scriptName << "(" << generateCSMethodParams(entry, false) << ")" << std::endl;
				methods << "\t\t{" << std::endl;

				bool returnByParam = false;
				if (!entry.returnInfo.type.empty())
				{
					if (!canBeReturned(returnTypeInfo.type, entry.returnInfo.flags))
					{
						methods << "\t\t\t" << returnType << " temp;" << std::endl;
						methods << "\t\t\tInternal_" << entry.interopName << "(";
						returnByParam = true;
					}
					else
						methods << "\t\t\treturn Internal_" << entry.interopName << "(";
				}
				else
					methods << "\t\t\tInternal_" << entry.interopName << "(";

				if (!isStatic)
					methods << "mCachedPtr";

				if (entry.paramInfos.size() > 0 || returnByParam)
					methods << ", ";

				methods << generateCSMethodArgs(entry, true);

				if (returnByParam)
				{
					if (entry.paramInfos.size() > 0)
						methods << ", ";

					methods << getCSVarType("temp", returnTypeInfo.type, entry.returnInfo.flags, true, false, true);
				}

				methods << ");" << std::endl;

				if (returnByParam)
					methods << "\t\t\treturn temp;" << std::endl;

				methods << "\t\t}" << std::endl;
				methods << std::endl;
			}
		}
	}

	// Properties
	for (auto& entry : input.propertyInfos)
	{
		UserTypeInfo propTypeInfo = getTypeInfo(entry.type, entry.typeFlags);
		std::string propTypeName = getCSVarType(propTypeInfo.scriptName, propTypeInfo.type, entry.typeFlags, false, true, false);

		properties << entry.documentation;
		properties << "\t\tpublic " << propTypeName << " " << entry.name << std::endl;
		properties << "\t\t{" << std::endl;

		if (!entry.getter.empty())
		{
			if (canBeReturned(propTypeInfo.type, entry.typeFlags))
				properties << "\t\t\tget { return Internal_" << entry.getter << "(mCachedPtr); }" << std::endl;
			else
			{
				properties << "\t\t\tget" << std::endl;
				properties << "\t\t\t{" << std::endl;
				properties << "\t\t\t\t" << propTypeName << " temp;" << std::endl;

				properties << "\t\t\t\tInternal_" << entry.getter << "(";

				if (!entry.isStatic)
					properties << "mCachedPtr, ";

				properties << "ref temp);" << std::endl;

				properties << "\t\t\t\treturn temp;" << std::endl;
				properties << "\t\t\t}" << std::endl;
			}
		}

		if (!entry.setter.empty())
		{
			properties << "\t\t\tset { Internal_" << entry.setter << "(";

			if (!entry.isStatic)
				properties << "mCachedPtr, ";

			properties << "value); }" << std::endl;
		}

		properties << "\t\t}" << std::endl;
		properties << std::endl;
	}

	std::stringstream output;
	output << input.documentation;

	if (input.visibility == CSVisibility::Internal)
		output << "\tinternal ";
	else if (input.visibility == CSVisibility::Public)
		output << "\tpublic ";
	else
		output << "\t";

	std::string baseType;
	if (!input.baseClass.empty())
	{
		UserTypeInfo baseTypeInfo = getTypeInfo(input.baseClass, 0);
		baseType = baseTypeInfo.scriptName;
	}
	else if (typeInfo.type == ParsedType::Resource)
		baseType = "Resource";
	else if (typeInfo.type == ParsedType::Component)
		baseType = "Component";
	else
		baseType = "ScriptObject";

	output << "partial class " << typeInfo.scriptName << " : " << baseType;

	output << std::endl;
	output << "\t{" << std::endl;

	output << ctors.str();
	output << properties.str();
	output << methods.str();
	output << interops.str();

	output << "\t}" << std::endl;
	return output.str();
}

std::string generateCSStruct(StructInfo& input)
{
	std::stringstream output;

	output << input.documentation;

	if (input.visibility == CSVisibility::Internal)
		output << "\tinternal ";
	else if (input.visibility == CSVisibility::Public)
		output << "\tpublic ";
	else
		output << "\t";

	std::string scriptName = cppToCsTypeMap[input.name].scriptName;
	output << "partial struct " << scriptName;

	output << std::endl;
	output << "\t{" << std::endl;

	for (auto& entry : input.ctors)
	{
		output << "\t\tpublic " << scriptName << "(";

		for (auto I = entry.params.begin(); I != entry.params.end(); ++I)
		{
			const VarInfo& paramInfo = *I;

			UserTypeInfo typeInfo = getTypeInfo(paramInfo.type, paramInfo.flags);

			if (!isValidStructType(typeInfo, paramInfo.flags))
			{
				// We report the error during field generation, as it checks for the same condition
				continue;
			}

			output << typeInfo.scriptName << " " << paramInfo.name;

			if (!paramInfo.defaultValue.empty())
				output << " = " << paramInfo.defaultValue;

			if ((I + 1) != entry.params.end())
				output << ", ";
		}

		output << ")" << std::endl;
		output << "\t\t{" << std::endl;

		for (auto I = entry.params.begin(); I != entry.params.end(); ++I)
		{
			const VarInfo& paramInfo = *I;

			UserTypeInfo typeInfo = getTypeInfo(paramInfo.type, paramInfo.flags);

			if (!isValidStructType(typeInfo, paramInfo.flags))
			{
				// We report the error during field generation, as it checks for the same condition
				continue;
			}

			auto iterFind = entry.fieldAssignments.find(I->name);
			if (iterFind == entry.fieldAssignments.end())
				continue;

			std::string fieldName = iterFind->first;
			std::string paramName = iterFind->second;

			output << "\t\t\tthis." << fieldName << " = " << paramName << ";" << std::endl;
		}

		output << "\t\t}" << std::endl;
		output << std::endl;
	}

	for (auto I = input.fields.begin(); I != input.fields.end(); ++I)
	{
		const VarInfo& fieldInfo = *I;

		UserTypeInfo typeInfo = getTypeInfo(fieldInfo.type, fieldInfo.flags);

		if (!isValidStructType(typeInfo, fieldInfo.flags))
		{
			errs() << "Invalid field type found in struct \"" << scriptName << "\" for field \"" << fieldInfo.name << "\". Skipping.\n";
			continue;
		}

		output << "\t\tpublic ";
		output << typeInfo.scriptName << " ";
		output << fieldInfo.name;

		output << ";" << std::endl;
	}

	output << "\t}" << std::endl;
	return output.str();
}

void cleanAndPrepareFolder(const StringRef& folder)
{
	if (sys::fs::exists(folder))
	{
		std::error_code ec;
		for (sys::fs::directory_iterator file(folder, ec), fileEnd; file != fileEnd && !ec; file.increment(ec))
			sys::fs::remove(file->path());
	}

	sys::fs::create_directories(folder);
}

std::ofstream createFile(const std::string& filename, FileType type, StringRef outputFolder)
{
	const std::string& folder = fileTypeFolders[(int)type];

	std::string relativePath = folder + "/" + filename;
	StringRef filenameRef(relativePath.data(), relativePath.size());

	SmallString<128> filepath = outputFolder;
	sys::path::append(filepath, filenameRef);

	std::ofstream output;
	output.open(filepath.str(), std::ios::out);

	return output;
}

void generateAll(StringRef cppOutputFolder, StringRef csEngineOutputFolder, StringRef csEditorOutputFolder)
{
	postProcessFileInfos();

	for (auto& folderName : fileTypeFolders)
	{
		StringRef filenameRef(folderName.data(), folderName.size());

		SmallString<128> folderPath = cppOutputFolder;
		sys::path::append(folderPath, filenameRef);

		cleanAndPrepareFolder(folderPath);
	}

	cleanAndPrepareFolder(csEngineOutputFolder);
	cleanAndPrepareFolder(csEditorOutputFolder);

	// Generate H
	for (auto& fileInfo : outputFileInfos)
	{
		std::stringstream body;

		auto& classInfos = fileInfo.second.classInfos;
		auto& structInfos = fileInfo.second.structInfos;

		for (auto I = classInfos.begin(); I != classInfos.end(); ++I)
		{
			ClassInfo& classInfo = *I;
			UserTypeInfo& typeInfo = cppToCsTypeMap[classInfo.name];

			body << generateCppHeaderOutput(classInfo, typeInfo);

			if ((I + 1) != classInfos.end() || !structInfos.empty())
				body << std::endl;
		}

		for (auto I = structInfos.begin(); I != structInfos.end(); ++I)
		{
			StructInfo& structInfo = *I;
			body << generateCppStructHeader(structInfo);

			if ((I + 1) != structInfos.end())
				body << std::endl;
		}

		FileType fileType = fileInfo.second.inEditor ? FT_EDITOR_H : FT_ENGINE_H;
		std::ofstream output = createFile("BsScript" + fileInfo.first + ".generated.h", fileType, cppOutputFolder);

		output << "#pragma once" << std::endl;
		output << std::endl;

		// Output includes
		for (auto& include : fileInfo.second.referencedHeaderIncludes)
			output << "#include \"" << include << "\"" << std::endl;

		output << std::endl;

		output << "namespace bs" << std::endl;
		output << "{" << std::endl;
		output << body.str();
		output << "}" << std::endl;

		output.close();
	}

	// Generate CPP
	for (auto& fileInfo : outputFileInfos)
	{
		std::stringstream body;

		auto& classInfos = fileInfo.second.classInfos;
		auto& structInfos = fileInfo.second.structInfos;

		for (auto I = classInfos.begin(); I != classInfos.end(); ++I)
		{
			ClassInfo& classInfo = *I;
			UserTypeInfo& typeInfo = cppToCsTypeMap[classInfo.name];

			body << generateCppSourceOutput(classInfo, typeInfo);

			if ((I + 1) != classInfos.end() || !structInfos.empty())
				body << std::endl;
		}

		for (auto I = structInfos.begin(); I != structInfos.end(); ++I)
		{
			body << generateCppStructSource(*I);

			if ((I + 1) != structInfos.end())
				body << std::endl;
		}

		FileType fileType = fileInfo.second.inEditor ? FT_EDITOR_CPP : FT_ENGINE_CPP;
		std::ofstream output = createFile("BsScript" + fileInfo.first + ".generated.cpp", fileType, cppOutputFolder);

		// Output includes
		for (auto& include : fileInfo.second.referencedSourceIncludes)
			output << "#include \"" << include << "\"" << std::endl;

		output << std::endl;

		output << "namespace bs" << std::endl;
		output << "{" << std::endl;
		output << body.str();
		output << "}" << std::endl;

		output.close();
	}

	// Generate CS
	for (auto& fileInfo : outputFileInfos)
	{
		std::stringstream body;

		auto& classInfos = fileInfo.second.classInfos;
		auto& structInfos = fileInfo.second.structInfos;
		auto& enumInfos = fileInfo.second.enumInfos;

		for (auto I = classInfos.begin(); I != classInfos.end(); ++I)
		{
			ClassInfo& classInfo = *I;
			UserTypeInfo& typeInfo = cppToCsTypeMap[classInfo.name];

			body << generateCSClass(classInfo, typeInfo);

			if ((I + 1) != classInfos.end() || !structInfos.empty() || !enumInfos.empty())
				body << std::endl;
		}

		for (auto I = structInfos.begin(); I != structInfos.end(); ++I)
		{
			body << generateCSStruct(*I);

			if ((I + 1) != structInfos.end() || !enumInfos.empty())
				body << std::endl;
		}

		for (auto I = enumInfos.begin(); I != enumInfos.end(); ++I)
		{
			body << I->code;

			if ((I + 1) != enumInfos.end())
				body << std::endl;
		}

		FileType fileType = fileInfo.second.inEditor ? FT_EDITOR_CS : FT_ENGINE_CS;
		StringRef outputFolder = fileInfo.second.inEditor ? csEditorOutputFolder : csEngineOutputFolder;
		std::ofstream output = createFile(fileInfo.first + ".generated.cs", fileType, outputFolder);

		output << "using System;" << std::endl;
		output << "using System.Runtime.CompilerServices;" << std::endl;
		output << "using System.Runtime.InteropServices;" << std::endl;

		if (fileInfo.second.inEditor)
			output << "using BansheeEngine;" << std::endl;

		output << std::endl;

		if (!fileInfo.second.inEditor)
			output << "namespace BansheeEngine" << std::endl;
		else
			output << "namespace BansheeEditor" << std::endl;

		output << "{" << std::endl;
		output << body.str();
		output << "}" << std::endl;

		output.close();
	}
}