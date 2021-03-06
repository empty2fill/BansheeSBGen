#include <string>
#include <vector>
#include <memory>

template<class Type>
class vector
{
	public:
	Type a;
};

template<class T, class S = unsigned int>
class Flags
{
	public:
	Flags(T v)
	{
		this->v = static_cast<S>(v);
	}
	
	S v;
};

using String = std::string;

namespace bs
{
	class GUIElement
	{ };
	
	class StringUtil
	{
	public:
		static const String BLANK;
	};

	const String StringUtil::BLANK;
}

typedef int INT32;

template<class Elem> class basic_string
{
	protected:
	Elem _elem;
};

typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;

#define BS_SCRIPT_EXPORT(...) __attribute__((annotate("se," #__VA_ARGS__)))
#define BS_PARAMS __attribute__((annotate("params")))

namespace bs
{
	class Component
	{
	public:
		virtual ~Component() {}
	};
	
	// START TEST DISTRIBUTION INITIALIZER
	
		/** Specifies a value as a distribution, which can include a constant value, random range or a curve. */
	template<class T>
	struct TDistribution
	{
		/** Creates a new empty distribution. */
		BS_SCRIPT_EXPORT()
		TDistribution();
		
		/** Creates a new distribution that returns a constant value. */
		BS_SCRIPT_EXPORT()
		TDistribution(T value);

		/** Creates a new distribution that returns a random value in the specified range. */
		BS_SCRIPT_EXPORT()
		TDistribution(T minValue, T maxValue);
	};

	using FloatDistribution = TDistribution<float>;
	template struct BS_SCRIPT_EXPORT(m:Particles,n:FloatDistribution) TDistribution<float>;

	struct BS_SCRIPT_EXPORT(pl:true) DistributionTest
	{
		DistributionTest() = default;
		DistributionTest(FloatDistribution count)
			:count(std::move(count))
		{ }
		
		FloatDistribution count = 0;
	};
	
	// END TEST DISTRIBUTION INITIALIZER
	
	// START TEST CORE VARIANTS
	template<class T>
	struct CoreThreadType
	{ };
	
	template<class T, bool Core>
	struct CoreVariant { };

	template<class T>
	struct CoreVariant<T, false> { typedef T Type; };

	template<class T> struct CoreVariant<T, true> { typedef typename CoreThreadType<T>::Type Type; };

	template<class T, bool Core>
	using CoreVariantType = typename CoreVariant<T, Core>::Type;

	struct BS_SCRIPT_EXPORT() ParticleVectorFieldSettings
	{
		/** Intensity of the forces and velocities applied by the vector field. */
		BS_SCRIPT_EXPORT()
		float intensity = 1.0f;
	};
	
	namespace ct
	{
		struct ParticleVectorFieldSettings {};
	}

	template<> struct CoreThreadType<ParticleVectorFieldSettings> { typedef ct::ParticleVectorFieldSettings Type; };
	
	class BS_SCRIPT_EXPORT() CoreVariantTest
	{
		BS_SCRIPT_EXPORT() CoreVariantType<ParticleVectorFieldSettings, false> vectorField;
	};
	// END TEST CORE VARIANTS
	
	// START TEST ComponentOrActor
	template<class T>
	struct GameObjectHandle { };

	template<class T>
	struct ComponentType { };

	class BS_SCRIPT_EXPORT() Renderable {};
	class BS_SCRIPT_EXPORT() CRenderable : public Component {};
	template<> struct ComponentType<Renderable> { typedef CRenderable Type; };

	template<class T>
	struct ComponentOrActor
	{
		using ComponentType = typename ComponentType<T>::Type;
		using HandleType = GameObjectHandle<ComponentType>;
		
		ComponentOrActor() = default;

		ComponentOrActor(const GameObjectHandle<ComponentType>& component)
			:mComponent(component)
		{ }

		ComponentOrActor(const std::shared_ptr<T>& actor)
			:mActor(actor)
		{ }

		/** Returns true if both the component and the actor fields are not assigned. */
		bool empty() const
		{
			return !mActor && !mComponent;
		}

		/** Returns the assigned value as a scene actor. */
		std::shared_ptr<T> getActor() const
		{
			if(mActor)
				return mActor;

			return mComponent->_getInternal();
		}

	private:
		GameObjectHandle<ComponentType> mComponent;
		std::shared_ptr<T> mActor;
	};

	class BS_SCRIPT_EXPORT() ComponentOrActorTest
	{
		BS_SCRIPT_EXPORT() ComponentOrActor<Renderable> renderable;
	};
	
	// END TEST ComponentOrActor
	
	// START TEST GameObjectHandle array
	
	class SceneObject;
	typedef GameObjectHandle<SceneObject> HSceneObject;
	
	class BS_SCRIPT_EXPORT() GOHandleArray
	{
		BS_SCRIPT_EXPORT() 
		void doSomething(std::vector<HSceneObject> stuff);
	};
		
	
	// END TEST GameObjectHandle array
	
	
template <class T>
class TCtorBase : public GUIElement
{
	BS_SCRIPT_EXPORT(ec:T)
	static T* create(int a);
};

class BS_SCRIPT_EXPORT() GUIElementTest : public TCtorBase<GUIElementTest>
{
	
};
	
template <class RetType, class... Args>
class TEvent
{
};

template<class Elem> class TResourceHandle
{
	
};

class Resource
{
	
};

class BS_SCRIPT_EXPORT() Texture : public Resource
{
	
};

template <typename Signature>
class Event;

/** @copydoc TEvent */
template <class RetType, class... Args>
class Event<RetType(Args...) > : public TEvent <RetType, Args...>
{ };

	struct Degree;

	class Radian
	{
	public:
		explicit Radian(float r = 0.0f) : mRad(r) {}
		Radian(const Degree& d);
		Radian& operator= (const float& f) { mRad = f; return *this; }
		Radian& operator= (const Radian& r) { mRad = r.mRad; return *this; }
		Radian& operator= (const Degree& d);

		/** Returns the value of the angle in degrees. */
		float valueDegrees() const;

		/** Returns the value of the angle in radians. */
		float valueRadians() const { return mRad; }

		/** Wraps the angle in [0, 2 *  PI) range. */
		Radian wrap();

        const Radian& operator+ () const { return *this; }
		Radian operator+ (const Radian& r) const { return Radian (mRad + r.mRad); }
		Radian operator+ (const Degree& d) const;
		Radian& operator+= (const Radian& r) { mRad += r.mRad; return *this; }
		Radian& operator+= (const Degree& d);
		Radian operator- () const { return Radian(-mRad); }
		Radian operator- (const Radian& r) const { return Radian (mRad - r.mRad); }
		Radian operator- (const Degree& d) const;
		Radian& operator-= (const Radian& r) { mRad -= r.mRad; return *this; }
		Radian& operator-= (const Degree& d);
		Radian operator* (float f) const { return Radian (mRad * f); }
        Radian operator* (const Radian& f) const { return Radian (mRad * f.mRad); }
		Radian& operator*= (float f) { mRad *= f; return *this; }
		Radian operator/ (float f) const { return Radian (mRad / f); }
		Radian& operator/= (float f) { mRad /= f; return *this; }

		friend Radian operator* (float lhs, const Radian& rhs) { return Radian(lhs * rhs.mRad); }
		friend Radian operator/ (float lhs, const Radian& rhs) { return Radian(lhs / rhs.mRad); }
		friend Radian operator+ (Radian& lhs, float rhs) { return Radian(lhs.mRad + rhs); }
		friend Radian operator+ (float lhs, const Radian& rhs) { return Radian(lhs + rhs.mRad); }
		friend Radian operator- (const Radian& lhs, float rhs) { return Radian(lhs.mRad - rhs); }
		friend Radian operator- (const float lhs, const Radian& rhs) { return Radian(lhs - rhs.mRad); }

		bool operator<  (const Radian& r) const { return mRad <  r.mRad; }
		bool operator<= (const Radian& r) const { return mRad <= r.mRad; }
		bool operator== (const Radian& r) const { return mRad == r.mRad; }
		bool operator!= (const Radian& r) const { return mRad != r.mRad; }
		bool operator>= (const Radian& r) const { return mRad >= r.mRad; }
		bool operator>  (const Radian& r) const { return mRad >  r.mRad; }

	private:
		float mRad;
	};
	
class Math
{
public:
	static constexpr float PI = 3.14f;	
};

class BS_SCRIPT_EXPORT(n:LocString,m:Localization) HString
{
public:
	/**
	 * Creates a new empty localized string.
	 *
	 * @param[in]	stringTableId	Unique identifier of the string table to retrieve the string from.
	 */
	BS_SCRIPT_EXPORT()
	HString(uint32_t stringTableId = 0);
};

#define BS_NORREF __attribute__((annotate("norref")))

typedef bs::TResourceHandle<bs::Texture> HTexture;

struct BS_SCRIPT_EXPORT(pl:true,m:GUI) GUIContentImages
{
	GUIContentImages() = default;
	
	GUIContentImages(const bs::TResourceHandle<bs::Texture>& image)
		:normal(image), hover(image), active(image), focused(image),
		normalOn(image), hoverOn(image), activeOn(image), focusedOn(image)
	{ }

	BS_NORREF HTexture normal;
	BS_NORREF bs::TResourceHandle<bs::Texture> hover;
	BS_NORREF bs::TResourceHandle<bs::Texture> active;
	BS_NORREF bs::TResourceHandle<bs::Texture> focused;
	BS_NORREF bs::TResourceHandle<bs::Texture> normalOn;
	BS_NORREF bs::TResourceHandle<bs::Texture> hoverOn;
	BS_NORREF bs::TResourceHandle<bs::Texture> activeOn;
	BS_NORREF bs::TResourceHandle<bs::Texture> focusedOn;
};

class BS_SCRIPT_EXPORT(pl:true,m:GUI) GUIContent
{
public:
	/**	Constructs an empty content. */
	GUIContent() = default;

	/**	Constructs content with just a string. */
	GUIContent(const HString& text)
		:text(text)
	{ }
	
	GUIContent(const HString& text, const HString& tooltip)
		: text(text), tooltip(tooltip)
	{ }	
	
	HString text;
	GUIContentImages images;
	HString tooltip;
};

struct Spring;

struct BS_SCRIPT_EXPORT(pl:true) LimitAngularRange
	{
		/** Constructs an empty limit. */
		LimitAngularRange()
		{ }

		/**
		 * Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
		 * 
		 * @param	lower		Lower angle of the limit. Must be less than @p upper.
		 * @param	upper		Upper angle of the limit. Must be more than @p lower.
		 * @param	contactDist	Distance from the limit at which it becomes active. Allows the solver to activate earlier
		 *						than the limit is reached to avoid breaking the limit. Specify -1 for the default.
		 */
		LimitAngularRange(Radian lower, Radian upper, float contactDist = -1.0f)
			:lower(lower), upper(upper)
		{ }

		/**
		 * Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the resitution
		 * parameter and will be pulled back towards the limit by the provided spring.
		 * 
		 * @param	lower		Lower angle of the limit. Must be less than @p upper.
		 * @param	upper		Upper angle of the limit. Must be more than @p lower.
		 * @param	spring		Spring that controls how are the bodies pulled back towards the limit when they breach it.
		 * @param	restitution	Controls how do objects react when the limit is reached, values closer to zero specify
		 *						non-ellastic collision, while those closer to one specify more ellastic (i.e bouncy)
		 *						collision. Must be in [0, 1] range.
		 */
		LimitAngularRange(Radian lower, Radian upper, const Spring& spring, float restitution = 0.0f)
			:lower(lower), upper(upper)
		{ }

		bool operator==(const LimitAngularRange& other) const
		{
			return lower == other.lower && upper == other.upper;
		}

		/** Lower angle of the limit. Must be less than #upper. */
		BS_SCRIPT_EXPORT(range:[0,360])
		Radian lower = Radian(0.0f);

		/** Upper angle of the limit. Must be less than #lower. */
		Radian upper = Radian(0.0f);
	};
}

////////////////////////////////////////

enum class __attribute__((annotate("se,pl:true,f:TestOutput"))) MyEnum
{
	a = 5,
	b = 8,
	c
};

enum class __attribute__((annotate("se,pl:true,f:TestOutput"))) MyEnum2
{
	a, b, c
};

enum __attribute__((annotate("se,pl:true,f:TestOutput"))) MyEnum3
{
	ME_A,
	ME_B,
	ME_C
};

struct __attribute__((annotate("se,pl:true,f:TestOutput"))) MyStruct
{
	int a;
	float b;
	float c = 10.0f;
};

struct __attribute__((annotate("se,pl:true,f:TestOutput"))) MyStruct2
{
	MyStruct2()
		:a(5), b(15.0f), c(10.0f)
	{ }
	
	MyStruct2(int a, float b, float c = 5.0f)
		:a(a), b(b), c(c)
	{
		this->a = a;
		this->b = b + c;
	}
	
	int a;
	float b;
	float c;
};

enum BS_SCRIPT_EXPORT() FlgEnum
{
	a, b, c
};

struct Vector3I
{
	int32_t x, y, z;
	
	constexpr Vector3I(int32_t x, int32_t y, int32_t z)
		: x(x), y(y), z(z)
		{ }
};

enum BS_SCRIPT_EXPORT(n:MeshTopology,m:Rendering) DrawOperationType
{
	DOT_POINT_LIST		BS_SCRIPT_EXPORT(n:PointList)		= 1,
	DOT_LINE_LIST		BS_SCRIPT_EXPORT(n:LineList)		= 2,
	DOT_LINE_STRIP		BS_SCRIPT_EXPORT(n:LineStrip)		= 3,
	DOT_TRIANGLE_LIST	BS_SCRIPT_EXPORT(n:TriangleList)	= 4,
	DOT_TRIANGLE_STRIP	BS_SCRIPT_EXPORT(n:TriangleStrip)	= 5,
	DOT_TRIANGLE_FAN	BS_SCRIPT_EXPORT(n:TriangleFan)		= 6
};

struct BS_SCRIPT_EXPORT(pl:true) SubMesh
{
	SubMesh()
		: indexOffset(0), indexCount(0), drawOp(DOT_TRIANGLE_LIST)
	{ }

	SubMesh(uint32_t indexOffset, uint32_t indexCount, DrawOperationType drawOp):
		indexOffset(indexOffset), indexCount(indexCount), drawOp(drawOp)
	{ }

	uint32_t indexOffset;
	uint32_t indexCount;
	DrawOperationType drawOp;
};

typedef Flags<FlgEnum> FlgEnums;

/** Animation keyframe, represented as an endpoint of a cubic hermite spline. */
template <class T>
struct TKeyframe
{
	T value; /**< Value of the key. */
	T inTangent; /**< Input tangent (going from the previous key to this one) of the key. */
	T outTangent; /**< Output tangent (going from this key to next one) of the key. */
	float time; /**< Position of the key along the animation spline. */
};

template class BS_SCRIPT_EXPORT(n:Keyframe,pl:true) TKeyframe<float>;

template <>
struct BS_SCRIPT_EXPORT(n:KeyframeInt,pl:true) TKeyframe<INT32>
{
	INT32 value; /**< Value of the key. */
	float time; /**< Position of the key along the animation spline. */
};

struct BS_SCRIPT_EXPORT(pl:true) KeyframeContainer
{
	TKeyframe<float> keyframe;
};

template <class T>
class TAnimationCurve
{
	BS_SCRIPT_EXPORT(n:Evaluate)
	T evaluate(float time, bool loop = true) const;
	
	BS_SCRIPT_EXPORT(n:KeyFrame,p:getter)
	const std::vector<TKeyframe<T>>& getKeyFrames() const;
};

template class BS_SCRIPT_EXPORT(n:AnimationCurve) TAnimationCurve<float>;

struct BS_SCRIPT_EXPORT(m:Animation,pl:true) RootMotion
{
	RootMotion() { }
	RootMotion(const TAnimationCurve<float>& position, const TAnimationCurve<float>& rotation)
		:position(position), rotation(rotation)
	{ }

	/** Animation curve representing the movement of the root bone. */
	TAnimationCurve<float> position;

	/** Animation curve representing the rotation of the root bone. */
	TAnimationCurve<float> rotation;
};

struct BS_SCRIPT_EXPORT(pl:true) Str1
{
	float a;
	int c;
};

class BS_SCRIPT_EXPORT(m:Image) ColorGradient
{
public:
	BS_SCRIPT_EXPORT()
	ColorGradient() = default;

	/** Test comments. */
	BS_SCRIPT_EXPORT()
	int evaluate(float t, float& factor) const;
	
	/** Test comments 2. */
	BS_SCRIPT_EXPORT()
	int evaluate(float t, int& factor) const;
};

struct BS_SCRIPT_EXPORT(pl:true) Str2 : public Str1
{
	int cda;
	BS_SCRIPT_EXPORT(ex:true)
	float cdb;
	std::wstring cdc;
};

class BS_SCRIPT_EXPORT() Cmp1 : public bs::Component
{

};

class BS_SCRIPT_EXPORT() Cmp2 : public Cmp1
{

};

class BS_SCRIPT_EXPORT(ed:true,m:GUIEditor) GUIColorGradient : public bs::GUIElement
{
public:
	BS_SCRIPT_EXPORT(ec:GUIColorGradient)
	static GUIColorGradient* create(const String& styleName = bs::StringUtil::BLANK);

	/**	Color gradient to display. */
	BS_SCRIPT_EXPORT(pr:setter,n:Gradient)
	void setGradient(const ColorGradient& colorGradient);
	
	/**
	* Calculates the total range covered by a set of curves.
	*
	* @param[in]	curves		Curves to calculate range for.
	* @param[out]	xMin		Minimum time value present in the curves.
	* @param[out]	xMax		Maximum time value present in the curves.
	* @param[out]	yMin		Minimum curve value present in the curves.
	* @param[out]	yMax		Maximum curve value present in the curves.
	*/
	static void calculateRange(const std::vector<TAnimationCurve<float>>& curves, 
		float& xMin, float& xMax, float& yMin, float& yMax);

	/** @copydoc calculateRange(const std::vector<TAnimationCurve<float>>&, float&, float&, float&, float&) */
	BS_SCRIPT_EXPORT()
	static void calculateRange(const std::vector<std::shared_ptr<TAnimationCurve<float>>>& curves, 
		float& xMin, float& xMax, float& yMin, float& yMax);
	
	/** @copydoc setGradient */
	BS_SCRIPT_EXPORT(pr:getter,n:Gradient)
	ColorGradient getGradient() const;

	BS_SCRIPT_EXPORT()
	bs::Event<void()> onClicked;
};

namespace bs
{
	class BS_SCRIPT_EXPORT(m:Math) Random
	{
	};

	typedef unsigned int RGBA;

	struct BS_SCRIPT_EXPORT(m:Particles) ColorDistribution
	{
		/** Creates a new empty distribution. */
		BS_SCRIPT_EXPORT()
		ColorDistribution();

		/** 
		 * Evaluates the value of the distribution.
		 * 
		 * @param[in]	t		Time at which to evaluate the distribution. This is only relevant if the distribution
		 *						contains gradients.
		 * @param[in]	factor	Value in range [0, 1] that determines how to interpolate between min/max value, if the
		 *						distribution represents a range. Value of 0 will return the minimum value, while value of 1
		 *						will return the maximum value, and interpolate the values in-between.
		 * @return				Evaluated color.
		 *
		 */
		RGBA evaluate(float t, float factor) const;

		/** 
		 * Evaluates the value of the distribution.
		 * 
		 * @param[in]	t		Time at which to evaluate the distribution. This is only relevant if the distribution
		 *						contains gradients.
		 * @param[in]	factor	Random number generator that determines the factor. Factor determines how to interpolate
		 *						between min/max value, if the distribution represents a range.
		 * @return				Evaluated color.
		 *
		 */
		RGBA evaluate(float t, Random& factor) const;
	};

	struct Color
	{};

	/** Extension class for ColorDistribution, for adding additional functionality for the script interface. */
	class BS_SCRIPT_EXPORT(e:ColorDistribution) ColorDistributionEx
	{
	public:
		/** @copydoc ColorDistribution::evaluate(float, float) */
		BS_SCRIPT_EXPORT(e:ColorDistribution)
		static Color evaluate(const std::shared_ptr<ColorDistribution>& thisPtr, float t, float factor);

		/** @copydoc ColorDistribution::evaluate(float, Random&) */
		BS_SCRIPT_EXPORT(e:ColorDistribution)
		static Color evaluate(const std::shared_ptr<ColorDistribution>& thisPtr, float t, Random& factor);
	}; 
}

enum class BS_SCRIPT_EXPORT(m:Resources) ResourceLoadFlag
{
	None = 0,
	LoadDependencies = 1 << 0, 
	KeepInternalRef = 1 << 1,
	KeepSourceData = 1 << 2,
	Default = LoadDependencies | KeepInternalRef
};

typedef Flags<ResourceLoadFlag> ResourceLoadFlags;

class BS_SCRIPT_EXPORT(f:TestOutput) MyClass
{
	public:
	/**
	 * @native
	 * Testing native docs!! @p initialData is a parameter!
	 *
	 * Multiparagaph!
	 * @endnative
	 * @script
	 * Creates a new mesh from an existing mesh data. Created mesh will match the vertex and index buffers described
	 * by the mesh data exactly. Mesh will have no sub-meshes.
	 * @endscript
	 *
	 * Use in both documentations
	 *
	 * @param[in]	initialData		Vertex and index data to initialize the mesh with.
	 * @param[in]	desc			Descriptor containing the properties of the mesh to create. Vertex and index count,
	 *								vertex descriptor and index type properties are ignored and are read from provided
	 *								mesh data instead.
	 * @returns						Mesh.
	 */
	BS_SCRIPT_EXPORT() int create(const int& initialData, const int& desc, unsigned long long superlong = 0xFFFFFFFFFFFFFFFF);
	
	BS_SCRIPT_EXPORT() void defParamTest(ResourceLoadFlags t = ResourceLoadFlag::Default);
	
	BS_SCRIPT_EXPORT() void strParamTest(String a = bs::StringUtil::BLANK);
	
	BS_SCRIPT_EXPORT() void paramsTest(int a, BS_PARAMS std::vector<float> b);
	
	BS_SCRIPT_EXPORT() ColorGradient anotherTest();
	
	/** Some stuff to comment @p dft some more stuff. */
	BS_SCRIPT_EXPORT() void tst(const Vector3I& dft = Vector3I(1, 1, 1));
	
	BS_SCRIPT_EXPORT()
	std::vector<bs::TResourceHandle<bs::Texture>> textures;
	
	BS_SCRIPT_EXPORT()
	std::vector<std::wstring> getIdentifiers() const;
	
	/** Some docs. */
	BS_SCRIPT_EXPORT()
	void setSomething(float t);
	
	/** @copydoc setSomething() */
	BS_SCRIPT_EXPORT()
	float getSomething() const;
	
	BS_SCRIPT_EXPORT()
	FlgEnums getEnum() const;	
	
	BS_SCRIPT_EXPORT()
	void setEnum(FlgEnums e) const;		
	
	BS_SCRIPT_EXPORT()
	bs::Event<void(int)> myEvent;
	
	BS_SCRIPT_EXPORT()
	bs::Event<void(FlgEnums)> myEnumEvent;
	
	BS_SCRIPT_EXPORT()
	static bs::Event<void(int)> myStaticEvent;
};

class BS_SCRIPT_EXPORT() StaticClass
{
	BS_SCRIPT_EXPORT()
	static StaticClass get();
	
	BS_SCRIPT_EXPORT()
	static void set(const StaticClass& v);
};

struct BS_SCRIPT_EXPORT(pl:true) MyStruct3
{
	float a;
	FlgEnums b;
	int c;
};

struct BS_SCRIPT_EXPORT(pl:true) MyStruct4
{
	MyStruct4() {}
	
	MyStruct4(MyStruct b)
		:a(b)
	{ }
	
	MyStruct a;
	std::string c;
};

template <class T>
struct TNamedAnimationCurve
{
	std::string name;
	T value;
};

template class BS_SCRIPT_EXPORT(m:Animation,n:NamedFloatCurve) TNamedAnimationCurve<float>;

struct BS_SCRIPT_EXPORT(pl:true) ComplexStruct3
{
	MyStruct2 a;
	std::shared_ptr<MyClass> b;
	float c;
	FlgEnums d;
	std::vector<int> e;
};

struct BS_SCRIPT_EXPORT(pl:true) ComplexStruct2
{
	MyStruct2 a;
	std::shared_ptr<MyClass> b;
	float c;
	std::vector<int> d;
};

struct BS_SCRIPT_EXPORT(pl:true) ComplexStruct
{
	MyStruct a;
	int b;
	std::string c;
	ComplexStruct2 d;
};

class BS_SCRIPT_EXPORT(f:TestOutput2,m:TestModule) MyClass2
{
	public:
	BS_SCRIPT_EXPORT()
	MyClass2(bool a, const std::shared_ptr<MyClass>& b = nullptr);
	
	BS_SCRIPT_EXPORT()
	ComplexStruct getStruct();
	
	BS_SCRIPT_EXPORT()
	void setStruct(const ComplexStruct& value);
	
	BS_SCRIPT_EXPORT()
	std::vector<ComplexStruct> getStructArr();
	
	BS_SCRIPT_EXPORT()
	void setStructArr(const std::vector<ComplexStruct>& value);
	
	BS_SCRIPT_EXPORT()
	bs::Event<void(ComplexStruct)> myEvent;
	
	BS_SCRIPT_EXPORT()
	bs::Event<void(std::vector<ComplexStruct>)> myEvent2;
	
};
