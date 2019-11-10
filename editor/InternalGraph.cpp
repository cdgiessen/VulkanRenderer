#include "InternalGraph.h"

#include "core/Logger.h"

namespace InternalGraph
{

InputLink::InputLink () : value (-1.0f) {}
InputLink::InputLink (float in) : value (in) {}
InputLink::InputLink (int in) : value (in) {}
InputLink::InputLink (cml::vec2f in) : value (in) {}
InputLink::InputLink (cml::vec3f in) : value (in) {}
InputLink::InputLink (cml::vec4f in) : value (in) {}


void InputLink::SetInputNode (NodeID id)
{
	handle.id = id;
	hasInputNode = true;
}

void InputLink::ResetInputNode ()
{
	handle.id = -1;
	hasInputNode = false;
	handle.handle = nullptr;
}

NodeID InputLink::GetInputNode () const { return handle.id; }

bool InputLink::HasInputNode () const { return hasInputNode; }

void InputLink::SetInputNodePointer (Node* node)
{
	hasInputNode = true;
	handle.handle = node;
}

void InputLink::ResetInputNodePointer ()
{
	hasInputNode = false;
	handle.handle = nullptr;
}

void InputLink::SetDataValue (LinkTypeVariants data) { value = data; }

LinkTypeVariants InputLink::GetValue () const { return value; }

LinkTypeVariants InputLink::GetValue (const int x, const int z) const
{

	if (hasInputNode)
		return handle.handle->GetValue (x, z);
	else
	{

		return value;
	}
}

void AddNodeInputLinks (std::vector<InputLink>& links, std::vector<LinkType> types)
{
	for (auto& t : types)
	{
		switch (t)
		{
			case LinkType::Float:
				links.push_back (0.0f);
				break;
			case LinkType::Int:
				links.push_back (0);
				break;
			case LinkType::Vec2:
				links.push_back (cml::vec2f (0));
				break;
			case LinkType::Vec3:
				links.push_back (cml::vec3f (0));
				break;
			case LinkType::Vec4:
				links.push_back (cml::vec4f (0));
				break;
			default:
				break;
		}
	}
}

Node::Node (NodeType in_type) : nodeType (in_type)
{
	outputType = LinkType::Float;

	switch (nodeType)
	{
		case InternalGraph::NodeType::Output:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Float, LinkType::Vec4, LinkType::Int, LinkType::Int, LinkType::Int, LinkType::Int });
			break;

		case InternalGraph::NodeType::Addition:
		case InternalGraph::NodeType::Subtraction:
		case InternalGraph::NodeType::Multiplication:
		case InternalGraph::NodeType::Division:
		case InternalGraph::NodeType::Power:
		case InternalGraph::NodeType::Max:
		case InternalGraph::NodeType::Min:
			AddNodeInputLinks (inputLinks, { LinkType::Float, LinkType::Float });

			break;

		case InternalGraph::NodeType::Blend:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });
			break;
		case InternalGraph::NodeType::Clamp:
			AddNodeInputLinks (inputLinks, { LinkType::Float, LinkType::Float, LinkType::Float });
			break;
		case InternalGraph::NodeType::Selector:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });
			break;

		case InternalGraph::NodeType::ConstantInt:
			outputType = LinkType::Int;
			inputLinks.push_back (InputLink (0));
			break;
		case InternalGraph::NodeType::ConstantFloat:
			inputLinks.push_back (InputLink (0.0f));
			break;
		case InternalGraph::NodeType::Invert:
			inputLinks.push_back (InputLink (0.0f));
			break;

		case InternalGraph::NodeType::TextureIndex:
			outputType = LinkType::Int;
			inputLinks.push_back (InputLink (0));
			break;

		case InternalGraph::NodeType::FractalReturnType:
			outputType = LinkType::Int;
			inputLinks.push_back (InputLink (0));
			break;
		case InternalGraph::NodeType::CellularReturnType:
			outputType = LinkType::Int;
			inputLinks.push_back (InputLink (0));
			break;

		case InternalGraph::NodeType::ValueNoise:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Int, LinkType::Float, LinkType::Int, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::SimplexNoise:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Int, LinkType::Float, LinkType::Int, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::PerlinNoise:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Int, LinkType::Float, LinkType::Int, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::CubicNoise:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Int, LinkType::Float, LinkType::Int, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::WhiteNoise:
			AddNodeInputLinks (inputLinks, { LinkType::Int, LinkType::Float });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::CellNoise:
			AddNodeInputLinks (inputLinks, { LinkType::Int, LinkType::Float, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case InternalGraph::NodeType::VoronoiNoise:
			AddNodeInputLinks (inputLinks, { LinkType::Int, LinkType::Float, LinkType::Float, LinkType::Int });
			isNoiseNode = true;
			break;

		case NodeType::ColorCreator:
			outputType = LinkType::Vec4;
			AddNodeInputLinks (
			    inputLinks, { LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });
			break;

		case NodeType::MonoGradient:
			AddNodeInputLinks (inputLinks,
			    { LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });

			break;

		default:
			break;
	}
}

Node::~Node ()
{
	if (myNoise)
	{
		delete myNoise;
	}
}

void Node::SetLinkValue (const int index, const LinkTypeVariants data)
{
	inputLinks.at (index).SetDataValue (data);
}

LinkTypeVariants Node::GetHeightMapValue (const int x, const int z) const
{
	return std::get<float> (inputLinks.at (0).GetValue (x, z)) * 2 - 1;
}

LinkTypeVariants Node::GetSplatMapValue (const int x, const int z) const
{
	return inputLinks.at (1).GetValue (x, z);
}

LinkTypeVariants Node::GetValue (const int x, const int z) const
{
	LinkTypeVariants retVal;
	// LinkTypeVariants reA, reB;
	float a, b, c, d, alpha;
	float value, lower, upper, smooth;

	switch (nodeType)
	{
		case InternalGraph::NodeType::None:
			break;

		case InternalGraph::NodeType::Addition:

			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return std::get<float> (inputLinks.at (0).GetValue (x, z)) +
					       std::get<float> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Int:
					return std::get<int> (inputLinks.at (0).GetValue (x, z)) +
					       std::get<int> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec2:
					return std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)) +
					       std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec3:
					return std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)) +
					       std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec4:
					return std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)) +
					       std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z));
					break;
				default:
					break;
			}
			break;

		case InternalGraph::NodeType::Subtraction:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return std::get<float> (inputLinks.at (0).GetValue (x, z)) -
					       std::get<float> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Int:
					return std::get<int> (inputLinks.at (0).GetValue (x, z)) -
					       std::get<int> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec2:
					return std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)) -
					       std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec3:
					return std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)) -
					       std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec4:
					return std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)) -
					       std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z));
					break;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Multiplication:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return std::get<float> (inputLinks.at (0).GetValue (x, z)) *
					       std::get<float> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Int:
					return std::get<int> (inputLinks.at (0).GetValue (x, z)) *
					       std::get<int> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec2:
					return std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)) *
					       std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec3:
					return std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)) *
					       std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec4:
					return std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)) *
					       std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z));
					break;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Division:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return std::get<float> (inputLinks.at (0).GetValue (x, z)) /
					       std::get<float> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Int:
					return std::get<int> (inputLinks.at (0).GetValue (x, z)) /
					       std::get<int> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec2:
					return std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)) /
					       std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec3:
					return std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)) /
					       std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z));
					break;
				case InternalGraph::LinkType::Vec4:
					return std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)) /
					       std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z));
					break;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Power:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return cml::pow (std::get<float> (inputLinks.at (0).GetValue (x, z)),
					    std::get<float> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Int:
					return cml::pow ((float)std::get<int> (inputLinks.at (0).GetValue (x, z)),
					    (float)std::get<int> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec2:
					return cml::pow (std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec3:
					return cml::pow (std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec4:
					return cml::pow (std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z)));
					break;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Max:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return cml::max (std::get<float> (inputLinks.at (0).GetValue (x, z)),
					    std::get<float> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Int:
					return cml::max (std::get<int> (inputLinks.at (0).GetValue (x, z)),
					    std::get<int> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec2:
					return cml::max (std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec3:
					return cml::max (std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec4:
					return cml::max (std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z)));
					break;
				default:
					break;
			}
			break;

		case InternalGraph::NodeType::Min:
			switch (outputType)
			{
				case InternalGraph::LinkType::None:
					break;
				case InternalGraph::LinkType::Float:
					return cml::min (std::get<float> (inputLinks.at (0).GetValue (x, z)),
					    std::get<float> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Int:
					return cml::min (std::get<int> (inputLinks.at (0).GetValue (x, z)),
					    std::get<int> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec2:
					return cml::min (std::get<cml::vec2f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec2f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec3:
					return cml::min (std::get<cml::vec3f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec3f> (inputLinks.at (1).GetValue (x, z)));
					break;
				case InternalGraph::LinkType::Vec4:
					return cml::min (std::get<cml::vec4f> (inputLinks.at (0).GetValue (x, z)),
					    std::get<cml::vec4f> (inputLinks.at (1).GetValue (x, z)));
					break;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Blend:
			switch (outputType)
			{
				case InternalGraph::LinkType::Float:
					a = std::get<float> (inputLinks.at (0).GetValue (x, z));
					b = std::get<float> (inputLinks.at (1).GetValue (x, z));
					alpha = std::get<float> (inputLinks.at (2).GetValue (x, z));
					retVal = alpha * b + (1 - alpha) * a;
					return retVal;
				default:
					break;
			}
			break;
		case InternalGraph::NodeType::Clamp:
			switch (outputType)
			{
				case InternalGraph::LinkType::Float:
					value = std::get<float> (inputLinks.at (0).GetValue (x, z));
					lower = std::get<float> (inputLinks.at (1).GetValue (x, z));
					upper = std::get<float> (inputLinks.at (2).GetValue (x, z));
					retVal = cml::clamp (value, lower, upper);
					return retVal;
				default:
					break;
			}
			break;

		case InternalGraph::NodeType::Selector:
			value = std::get<float> (inputLinks.at (0).GetValue (x, z));
			a = std::get<float> (inputLinks.at (1).GetValue (x, z));
			b = std::get<float> (inputLinks.at (2).GetValue (x, z));
			lower = std::get<float> (inputLinks.at (3).GetValue (x, z));
			upper = std::get<float> (inputLinks.at (4).GetValue (x, z));
			smooth = std::get<float> (inputLinks.at (5).GetValue (x, z));

			if (smooth == 0)
			{
				if (value < lower && value > upper)
					return a;
				else
					return b;
			}
			if (value < lower - smooth / 2.0f)
			{
				return a;
			}
			else if (value >= lower - smooth / 2.0f && value < lower + smooth / 2.0f)
			{
				return ((value - (lower - smooth / 2.0f)) / smooth) * b +
				       (1 - ((value - (lower - smooth / 2.0f)) / smooth)) * a;
			}
			else if (value >= lower + smooth / 2.0f && value <= upper - smooth / 2.0f)
			{
				return b;
			}
			else if (value > upper - smooth / 2.0f && value <= upper + smooth / 2.0f)
			{
				return (((upper + smooth / 2.0f) - value) / smooth) * b +
				       (1 - (((upper + smooth / 2.0f) - value) / smooth)) * a;
			}
			else if (value > upper + smooth / 2.0f)
				return a;
			break;
		case InternalGraph::NodeType::ConstantInt:
			return inputLinks.at (0).GetValue (x, z);

			break;
		case InternalGraph::NodeType::ConstantFloat:
			return inputLinks.at (0).GetValue (x, z);

			break;

		case InternalGraph::NodeType::Invert:
			value = std::get<float> (inputLinks.at (0).GetValue (x, z));
			retVal = 1 - value;
			return retVal;

			break;

		case InternalGraph::NodeType::TextureIndex:
			return inputLinks.at (0).GetValue (x, z);

			break;

		case InternalGraph::NodeType::FractalReturnType:
			return inputLinks.at (0).GetValue (x, z);

		case InternalGraph::NodeType::CellularReturnType:
			return inputLinks.at (0).GetValue (x, z);

			break;

		case InternalGraph::NodeType::ValueNoise:
		case InternalGraph::NodeType::SimplexNoise:
		case InternalGraph::NodeType::PerlinNoise:
		case InternalGraph::NodeType::WhiteNoise:
		case InternalGraph::NodeType::CellNoise:
		case InternalGraph::NodeType::CubicNoise:
		case InternalGraph::NodeType::VoronoiNoise:

			retVal = (noiseImage.BoundedLookUp (x, z) + 1.0f) / 2.0f;
			return retVal;
			break;

		case NodeType::ColorCreator:

			a = std::get<float> (inputLinks.at (0).GetValue (x, z));
			b = std::get<float> (inputLinks.at (1).GetValue (x, z));
			c = std::get<float> (inputLinks.at (2).GetValue (x, z));
			d = std::get<float> (inputLinks.at (3).GetValue (x, z));

			retVal = cml::vec4f (a, b, c, d);

			return retVal;
			break;

		case NodeType::MonoGradient:
			value = std::get<float> (inputLinks.at (0).GetValue (x, z));
			lower = std::get<float> (inputLinks.at (1).GetValue (x, z));
			upper = std::get<float> (inputLinks.at (2).GetValue (x, z));
			smooth = std::get<float> (inputLinks.at (3).GetValue (x, z));
			// value = cml::clamp(value, lower, upper);
			retVal = lower + value * (upper - lower);
			// retVal = ((upper - value) + (value - lower)) / (upper - lower);

			return retVal;

			break;

		default:
			break;
	}
	return retVal;
}

NodeType Node::GetNodeType () const { return nodeType; }

LinkType Node::GetOutputType () const { return outputType; }


void Node::SetLinkInput (const int index, const NodeID id)
{
	inputLinks.at (index).SetInputNode (id);
}

void Node::ResetLinkInput (const int index) { inputLinks.at (index).ResetInputNode (); }

void Node::SetID (NodeID id) { this->id = id; }
NodeID Node::GetID () { return id; }

void Node::SetFractalType (int val)
{
	if (val == 2)
		myNoise->SetFractalType (FastNoiseSIMD::FractalType::RigidMulti);
	else if (val == 1)
		myNoise->SetFractalType (FastNoiseSIMD::FractalType::Billow);
	else
		myNoise->SetFractalType (FastNoiseSIMD::FractalType::FBM);
}

void Node::SetCellularDistanceFunction (int index)
{
	if (index == 2)
		myNoise->SetCellularDistanceFunction (FastNoiseSIMD::CellularDistanceFunction::Natural);
	else if (index == 1)
		myNoise->SetCellularDistanceFunction (FastNoiseSIMD::CellularDistanceFunction::Manhattan);
	else
		myNoise->SetCellularDistanceFunction (FastNoiseSIMD::CellularDistanceFunction::Euclidean);
}
void Node::SetCellularReturnType (int index)
{
	if (index == 1)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance);
	else if (index == 2)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2);
	else if (index == 3)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2Add);
	else if (index == 4)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2Sub);
	else if (index == 5)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2Mul);
	else if (index == 6)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2Div);
	else if (index == 7)
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::Distance2Cave);
	else
		myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::CellValue);
}

void Node::SetupInputLinks (NodeMap* map)
{
	for (auto& link : inputLinks)
	{
		if (link.HasInputNode ())
		{
			Node* ptr = &(map->at (link.GetInputNode ()));
			link.SetInputNodePointer (ptr);
		}
	}
}

void Node::SetupNodeForComputation (NoiseSourceInfo info)
{
	if (isNoiseNode)
	{
		myNoise = FastNoiseSIMD::NewFastNoiseSIMD ();

		myNoise->SetSeed (std::get<int> (inputLinks.at (0).GetValue ()));

		myNoise->SetFrequency (std::get<float> (inputLinks.at (1).GetValue ()));

		// myNoise->SetAxisScales(info.scale, info.scale, info.scale);
		switch (nodeType)
		{
			case InternalGraph::NodeType::WhiteNoise:
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetWhiteNoiseSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::ValueNoise:
				myNoise->SetFractalOctaves (std::get<int> (inputLinks.at (2).GetValue ()));
				myNoise->SetFractalGain (std::get<float> (inputLinks.at (3).GetValue ()));
				SetFractalType (std::get<int> (inputLinks.at (4).GetValue ()));
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetValueFractalSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::SimplexNoise:
				myNoise->SetFractalOctaves (std::get<int> (inputLinks.at (2).GetValue ()));
				myNoise->SetFractalGain (std::get<float> (inputLinks.at (3).GetValue ()));
				SetFractalType (std::get<int> (inputLinks.at (4).GetValue ()));
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetSimplexFractalSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::PerlinNoise:
				myNoise->SetFractalOctaves (std::get<int> (inputLinks.at (2).GetValue ()));
				myNoise->SetFractalGain (std::get<float> (inputLinks.at (3).GetValue ()));
				SetFractalType (std::get<int> (inputLinks.at (4).GetValue ()));
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetPerlinFractalSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::CubicNoise:
				myNoise->SetFractalOctaves (std::get<int> (inputLinks.at (2).GetValue ()));
				myNoise->SetFractalGain (std::get<float> (inputLinks.at (3).GetValue ()));
				SetFractalType (std::get<int> (inputLinks.at (4).GetValue ()));
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetCubicFractalSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::CellNoise:
				myNoise->SetCellularJitter (std::get<float> (inputLinks.at (2).GetValue ()));
				SetCellularReturnType (std::get<int> (inputLinks.at (3).GetValue ()));
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetCellularSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::VoronoiNoise:
				myNoise->SetCellularJitter (std::get<float> (inputLinks.at (2).GetValue ()));
				myNoise->SetCellularReturnType (FastNoiseSIMD::CellularReturnType::CellValue);
				noiseImage.SetImage (info.cellsWide,
				    myNoise->GetCellularSet (
				        info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			default:
				break;
		}
	}
}


GraphPrototype::GraphPrototype () {}

void GraphPrototype::ResetGraph ()
{
	nodeMap.clear ();
	nodeIDCounter = 0;
	outputNodeID = 0;
}

NodeID GraphPrototype::AddNode (NodeType type)
{
	int nextId = GetNextID ();

	nodeMap.emplace (std::piecewise_construct, std::forward_as_tuple (nextId), std::forward_as_tuple (type));
	nodeMap.at (nextId).SetID (nextId);

	if (nodeMap.at (nextId).GetNodeType () == NodeType::Output)
	{
		outputNodeID = nodeMap.at (nextId).GetID ();
	}
	return nextId;
}

bool GraphPrototype::DeleteNode (NodeID id)
{
	auto val = nodeMap.find (id);
	if (val != nodeMap.end ())
	{
		nodeMap.erase (id);
		return true;
	}
	else
		return false;
}

Node& GraphPrototype::GetNodeByID (NodeID id) { return nodeMap.at (id); }
NodeID GraphPrototype::GetNextID () { return nodeIDCounter++; }

NodeID GraphPrototype::GetOutputNodeID () const { return outputNodeID; }

void GraphPrototype::SetOutputNodeID (NodeID id) { outputNodeID = id; }

NodeMap GraphPrototype::GetNodeMap () const { return nodeMap; }


GraphUser::GraphUser (
    const GraphPrototype& graph, int seed, int cellsWide, cml::vec2<int32_t> pos, float scale, float height_scale)
: nodeMap (graph.GetNodeMap ()), info (seed, cellsWide, scale, pos)
{
	// cml::vec2<int32_t>(pos.x * (cellsWide) / scale, pos.y * (cellsWide) / scale), scale / (cellsWide)

	for (auto& node : nodeMap)
	{
		for (auto& link : node.second.inputLinks)
		{
			if (link.HasInputNode ())
			{
				Node* n = &nodeMap.at (link.GetInputNode ());
				link.SetInputNodePointer (n);
			}
		}
	}

	for (auto& node : nodeMap)
	{
		node.second.SetupNodeForComputation (info);
	}

	Node& outputNode = nodeMap[graph.GetOutputNodeID ()];

	outputHeightMap.reserve (cellsWide * cellsWide);
	outputSplatMap.reserve (cellsWide * cellsWide);
	outputNormalMap.reserve (cellsWide * cellsWide);

	for (int x = 0; x < cellsWide; x++)
	{
		for (int z = 0; z < cellsWide; z++)
		{
			float val = std::get<float> (outputNode.GetHeightMapValue (x, z));
			outputHeightMap.push_back (val);
		}
	}


	for (int x = 0; x < cellsWide; x++)
	{
		for (int z = 0; z < cellsWide; z++)
		{

			cml::vec4f val = std::get<cml::vec4f> (outputNode.GetSplatMapValue (z, x));

			assert (!std::isnan (val.x));

			outputSplatMap.push_back ({ static_cast<uint8_t> (val.x * 255),
			    static_cast<uint8_t> (val.y * 255),
			    static_cast<uint8_t> (val.z * 255),
			    static_cast<uint8_t> (val.w * 255) });
		}
	}

	for (int x = 0; x < cellsWide; x++)
	{
		for (int z = 0; z < cellsWide; z++)
		{

			if (x > 0 && x < cellsWide - 1 && z > 0 && z < cellsWide - 1)
			{


				// float h = outputHeightMap.at ((x)*cellsWide + z);
				float h_px = outputHeightMap.at ((x)*cellsWide + z + 1) * height_scale;
				float h_mx = outputHeightMap.at ((x)*cellsWide + z - 1) * height_scale;
				float h_py = outputHeightMap.at ((x + 1) * cellsWide + z) * height_scale;
				float h_my = outputHeightMap.at ((x - 1) * cellsWide + z) * height_scale;

				cml::vec3f normal = cml::normalize (cml::vec3f (h_px - h_mx, 2.0f, h_py - h_my));

				int16_t n_x = static_cast<int16_t> (normal.x * 32768);
				int16_t n_y = static_cast<int16_t> (normal.y * 32768);
				int16_t n_z = static_cast<int16_t> (normal.z * 32768);
				outputNormalMap.push_back (cml::vec4<int16_t>{ n_x, n_y, n_z, 0 });
			}
			else
				outputNormalMap.push_back ({ (int16_t)0.5, (int16_t)1, (int16_t)0.5, (int16_t)0 });
		}
	}
}


std::vector<float>& GraphUser::GetHeightMap () { return outputHeightMap; }
std::vector<cml::vec4<uint8_t>>& GraphUser::GetSplatMap () { return outputSplatMap; }
std::vector<cml::vec4<int16_t>>& GraphUser::GetNormalMap () { return outputNormalMap; }


float GraphUser::SampleHeightMap (const float x, const float z) const
{
	return 0.f;
	// TODO:
	// return BilinearImageSample2D (outputHeightMap, x, z);
}

} // namespace InternalGraph