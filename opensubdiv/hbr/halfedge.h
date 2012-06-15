//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//
#ifndef HBRHALFEDGE_H
#define HBRHALFEDGE_H

#include <assert.h>
#include <cstring>
#include <iostream>


#ifdef HBRSTITCH
#include "libgprims/stitch.h"
#include "libgprims/stitchInternal.h"
#endif

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrFace;
template <class T> class HbrHalfedge;
template <class T> class HbrVertex;
template <class T> class HbrMesh;

template <class T> std::ostream& operator<<(std::ostream& out, const HbrHalfedge<T>& edge);

template <class T> class HbrHalfedge {

public:

    HbrHalfedge(): opposite(0), incidentFace(0), incidentVertex(0), vchild(0), sharpness(0.0f)
#ifdef HBRSTITCH
    , stitchccw(1), raystitchccw(1)
#endif
    , coarse(1)
    {
    }

    ~HbrHalfedge();

    void Clear();
    
    // Finish the initialization of the halfedge. Should only be
    // called by HbrFace
    void Initialize(HbrHalfedge<T>* opposite, int index, HbrVertex<T>* origin, unsigned int *fvarbits, HbrFace<T>* face);

    // Returns the opposite half edge
    HbrHalfedge<T>* GetOpposite() const { return opposite; }

    // Sets the opposite half edge
    void SetOpposite(HbrHalfedge<T>* opposite) { this->opposite = opposite; sharpness = opposite->sharpness; }

    // Returns the next clockwise halfedge around the incident face
    HbrHalfedge<T>* GetNext() const {
        if (lastedge) {
            return (HbrHalfedge<T>*) ((char*) this - (incidentFace->GetNumVertices() - 1) * sizeof(HbrHalfedge<T>));
        } else {
            return (HbrHalfedge<T>*) ((char*) this + sizeof(HbrHalfedge<T>));
        }
    }

    // Returns the previous counterclockwise halfedge around the incident face
    HbrHalfedge<T>* GetPrev() const {
        if (firstedge) {
            return (HbrHalfedge<T>*) ((char*) this + (incidentFace->GetNumVertices() - 1) * sizeof(HbrHalfedge<T>));
        } else {
            return (HbrHalfedge<T>*) ((char*) this - sizeof(HbrHalfedge<T>));
        }
    }

    // Returns the index of the edge relative to its incident face.
    // This relies on knowledge of the face's edge allocation pattern
    int GetIndex() const {
        // we allocate room for up to 4 values (to handle tri or quad)
        // in the edges array.  If there are more than that, they _all_
        // go in the extra edges array.
        if (this >= incidentFace->edges &&
            this < incidentFace->edges + 4) {
            return this - incidentFace->edges;
        } else {
            return this - incidentFace->extraedges;
        }
    }
    
    // Returns the incident vertex
    HbrVertex<T>* GetVertex() const {
        return incidentVertex;
    }

    // Returns the source vertex
    HbrVertex<T>* GetOrgVertex() const {
        return incidentVertex;
    }

    // Changes the origin vertex. Generally not a good idea to do
    void SetOrgVertex(HbrVertex<T>* v) { incidentVertex = v; }
    
    // Returns the destination vertex
    HbrVertex<T>* GetDestVertex() const { return GetNext()->GetOrgVertex(); }
    
    // Returns the incident facet
    HbrFace<T>* GetFace() const { return incidentFace; }

    // Returns the mesh to which this edge belongs
    HbrMesh<T>* GetMesh() const { return incidentFace->GetMesh(); }
    
    // Returns the face on the right
    HbrFace<T>* GetRightFace() const { return opposite ? opposite->GetLeftFace() : NULL; }

    // Return the face on the left of the halfedge
    HbrFace<T>* GetLeftFace() const { return incidentFace; }

    // Returns whether this is a boundary edge
    bool IsBoundary() const { return opposite == 0; }

    // Tag the edge as being an infinitely sharp facevarying edge
    void SetFVarInfiniteSharp(int datum, bool infsharp) {
        int intindex = datum >> 4;
        unsigned int bits = infsharp << ((datum & 15) * 2);
        getFVarInfSharp()[intindex] |= bits;
	if (opposite) {
            opposite->getFVarInfSharp()[intindex] |= bits;
        }
    }

    // Copy fvar infinite sharpness flags from another edge
    void CopyFVarInfiniteSharpness(HbrHalfedge<T>* edge) {
        unsigned int *fvarinfsharp = getFVarInfSharp();
        if (fvarinfsharp) {
            const int fvarcount = GetMesh()->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            memcpy(fvarinfsharp, edge->getFVarInfSharp(), fvarbitsSizePerEdge * sizeof(unsigned int));
        }
    }
    
    // Returns whether the edge is infinitely sharp in facevarying for
    // a particular facevarying datum
    bool GetFVarInfiniteSharp(int datum);

    // Returns whether the edge is infinitely sharp in any facevarying
    // datum
    bool IsFVarInfiniteSharpAnywhere();

    // Get the sharpness relative to facevarying data
    float GetFVarSharpness(int datum, bool ignoreGeometry=false);
    
    // Returns the (raw) sharpness of the edge
    float GetSharpness() const { return sharpness; }

    // Sets the sharpness of the edge
    void SetSharpness(float sharp) { sharpness = sharp; if (opposite) opposite->sharpness = sharp; ClearMask(); }

    // Returns whether the edge is sharp at the current level of
    // subdivision (next = false) or at the next level of subdivision
    // (next = true).
    bool IsSharp(bool next) const { return (next ? (sharpness > 0.0f) : (sharpness >= 1.0f)); }
    
    // Clears the masks of the adjacent edge vertices. Usually called
    // when a change in edge sharpness occurs.
    void ClearMask() { GetOrgVertex()->ClearMask(); GetDestVertex()->ClearMask(); }

    // Subdivide the edge into a vertex if needed and return
    HbrVertex<T>* Subdivide();

    // Make sure the edge has its opposite face
    void GuaranteeNeighbor();
    
    // Remove the reference to subdivided vertex
    void RemoveChild() { vchild = 0; }

    // Sharpness constants
    enum Mask {
	k_Smooth = 0,
	k_Sharp = 1,
	k_InfinitelySharp = 10
    };

#ifdef HBRSTITCH
    StitchEdge* GetStitchEdge(int i) {
        StitchEdge **stitchEdge = getStitchEdges();
	// If the stitch edge exists, the ownership is transferred to
	// the caller. Make sure the opposite edge loses ownership as
	// well.
	if (stitchEdge[i]) {
	    if (opposite) {
		opposite->getStitchEdges()[i] = 0;
	    }
	    return StitchGetEdge(&stitchEdge[i]);
	}
	// If the stitch edge does not exist then we create one now.
	// Make sure the opposite edge gets a copy of it too
	else {
	    StitchGetEdge(&stitchEdge[i]);
	    if (opposite) {
		opposite->getStitchEdges()[i] = stitchEdge[i];
	    }
	    return stitchEdge[i];
	}
    }

    // If stitch edge exists, and this edge has no opposite, destroy
    // it
    void DestroyStitchEdges(int stitchcount) {
        if (!opposite) {
            StitchEdge **stitchEdge = getStitchEdges();
            for (int i = 0; i < stitchcount; ++i) {
                if (stitchEdge[i]) {
                    StitchFreeEdge(stitchEdge[i]);
                    stitchEdge[i] = 0;
                }
            }
        }
    }
    
    StitchEdge* GetRayStitchEdge(int i) {
	return GetStitchEdge(i + 2);
    }

    // Splits our split edge between our children. We'd better have
    // subdivided this edge by this point
    void SplitStitchEdge(int i) {
	StitchEdge* se = GetStitchEdge(i);
	HbrHalfedge<T>* ea = GetOrgVertex()->Subdivide()->GetEdge(Subdivide());
	HbrHalfedge<T>* eb = Subdivide()->GetEdge(GetDestVertex()->Subdivide());
        StitchEdge **ease = ea->getStitchEdges();
        StitchEdge **ebse = eb->getStitchEdges();
	if (i >= 2) { // ray tracing stitches
	    if (!raystitchccw) {
		StitchSplitEdge(se, &ease[i], &ebse[i], false, 0, 0, 0);
	    } else {
		StitchSplitEdge(se, &ebse[i], &ease[i], true, 0, 0, 0);
	    }
	    ea->raystitchccw = eb->raystitchccw = raystitchccw;
	    if (eb->opposite) {
		eb->opposite->getStitchEdges()[i] = ebse[i];
		eb->opposite->raystitchccw = raystitchccw;
	    }
	    if (ea->opposite) {
		ea->opposite->getStitchEdges()[i] = ease[i];
		ea->opposite->raystitchccw = raystitchccw;
	    }
	} else {
	    if (!stitchccw) {
		StitchSplitEdge(se, &ease[i], &ebse[i], false, 0, 0, 0);
	    } else {
		StitchSplitEdge(se, &ebse[i], &ease[i], true, 0, 0, 0);
	    }
	    ea->stitchccw = eb->stitchccw = stitchccw;
	    if (eb->opposite) {
		eb->opposite->getStitchEdges()[i] = ebse[i];
		eb->opposite->stitchccw = stitchccw;
	    }
	    if (ea->opposite) {
		ea->opposite->getStitchEdges()[i] = ease[i];
		ea->opposite->stitchccw = stitchccw;
	    }
	}
    }

    void SplitRayStitchEdge(int i) {
	SplitStitchEdge(i + 2);
    }
    
    void SetStitchEdge(int i, StitchEdge* edge) {
        StitchEdge **stitchEdges = getStitchEdges();
	stitchEdges[i] = edge;
	if (opposite) {
	    opposite->getStitchEdges()[i] = edge;
	}
    }

    void SetRayStitchEdge(int i, StitchEdge* edge) {
        StitchEdge **stitchEdges = getStitchEdges();
	stitchEdges[i+2] = edge;
	if (opposite) {
	    opposite->getStitchEdges()[i+2] = edge;
	}
    }

    void* GetStitchData() const {
        if (stitchdatavalid) return *(incidentFace->stitchDatas + GetIndex());
        else return 0;
    }

    void SetStitchData(void* data) {
        *(incidentFace->stitchDatas + GetIndex()) = data;
        stitchdatavalid = data ? 1 : 0;
	if (opposite) {
	    *(opposite->incidentFace->stitchDatas + opposite->GetIndex()) = data;
            opposite->stitchdatavalid = stitchdatavalid;
	}
    }
    
    bool GetStitchCCW(bool raytraced) const { return raytraced ? raystitchccw : stitchccw; }
    
    void ClearStitchCCW(bool raytraced) {
	if (raytraced) {
	    raystitchccw = 0;
	    if (opposite) opposite->raystitchccw = 0;
	} else {
	    stitchccw = 0;
	    if (opposite) opposite->stitchccw = 0;
	}
    }

    void ToggleStitchCCW(bool raytraced) {
	if (raytraced) {
	    raystitchccw = 1 - raystitchccw;
	    if (opposite) opposite->raystitchccw = raystitchccw;
	} else {
	    stitchccw = 1 - stitchccw;
	    if (opposite) opposite->stitchccw = stitchccw;
	}
    }

#endif

    // Marks the edge as being "coarse" (belonging to the control
    // mesh). Generally this distinction only needs to be made if
    // we're worried about interpolateboundary behaviour
    void SetCoarse(bool c) { coarse = c; }
    bool IsCoarse() const { return coarse; }

private:
    HbrHalfedge<T>* opposite;
    HbrFace<T>* incidentFace;

    // Index of incident vertex
    HbrVertex<T>* incidentVertex;

    // Index of child vertex
    HbrVertex<T>* vchild;
    float sharpness;

#ifdef HBRSTITCH
    unsigned char stitchccw:1;
    unsigned char raystitchccw:1;
    unsigned char stitchdatavalid:1;
#endif
    unsigned char coarse:1;
    unsigned char lastedge:1;
    unsigned char firstedge:1;
    
    // Returns bitmask indicating whether a given facevarying datum
    // for the edge is infinitely sharp. Each datum has two bits, and
    // if those two bits are set to 3, it means the status has not
    // been computed yet.
    unsigned int *getFVarInfSharp() {
        unsigned int *fvarbits = incidentFace->fvarbits;
        if (fvarbits) {
            int fvarbitsSizePerEdge = ((GetMesh()->GetFVarCount() + 15) / 16);
            return fvarbits + GetIndex() * fvarbitsSizePerEdge;
        } else {
            return 0;
        }
    }

#ifdef HBRSTITCH
    StitchEdge **getStitchEdges() {
        return incidentFace->stitchEdges + GetMesh()->GetStitchCount() * GetIndex();
    }
#endif
};

template <class T>
void
HbrHalfedge<T>::Initialize(HbrHalfedge<T>* opposite, int index, HbrVertex<T>* origin, unsigned int *fvarbits, HbrFace<T>* face) {
    this->opposite = opposite;
    incidentVertex = origin;
    incidentFace = face;
    lastedge = (index == face->GetNumVertices() - 1);
    firstedge = (index == 0);
    if (opposite) {
	sharpness = opposite->sharpness;
#ifdef HBRSTITCH
        StitchEdge **stitchEdges = getStitchEdges();
	for (int i = 0; i < face->GetMesh()->GetStitchCount(); ++i) {
	    stitchEdges[i] = opposite->getStitchEdges()[i];
	}
	stitchccw = opposite->stitchccw;
	raystitchccw = opposite->raystitchccw;
        stitchdatavalid = 0;
        if (stitchEdges && opposite->GetStitchData()) {
            *(incidentFace->stitchDatas + index) = opposite->GetStitchData();
            stitchdatavalid = 1;
        }
#endif
        if (fvarbits) {
            const int fvarcount = face->GetMesh()->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            memcpy(fvarbits, opposite->getFVarInfSharp(), fvarbitsSizePerEdge * sizeof(unsigned int));
        }
    } else {
        sharpness = 0.0f;
#ifdef HBRSTITCH
        StitchEdge **stitchEdges = getStitchEdges();
	for (int i = 0; i < face->GetMesh()->GetStitchCount(); ++i) {
	    stitchEdges[i] = 0;
	}
        stitchccw = 1;
        raystitchccw = 1;
        stitchdatavalid = 0;
#endif
        if (fvarbits) {
            const int fvarcount = face->GetMesh()->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            memset(fvarbits, 0xff, fvarbitsSizePerEdge * sizeof(unsigned int));
        }
    }    
}

template <class T>
HbrHalfedge<T>::~HbrHalfedge() {
    Clear();
}

template <class T>
void
HbrHalfedge<T>::Clear() {
    if (opposite) {
	opposite->opposite = 0;
	if (vchild) {
	    // Transfer ownership of the vchild to the opposite ptr
	    opposite->vchild = vchild;
            // Done this way just for assertion sanity
            vchild->SetParent(static_cast<HbrHalfedge*>(0));
            vchild->SetParent(opposite);
            vchild = 0;
	}
	opposite = 0;
    }
    // Orphan the child vertex
    else if (vchild) {
        vchild->SetParent(static_cast<HbrHalfedge*>(0));
	vchild = 0;
    }
}

template <class T>
HbrVertex<T>*
HbrHalfedge<T>::Subdivide() {
    if (vchild) return vchild;
    // Make sure that our opposite doesn't "own" a subdivided vertex
    // already. If it does, use that
    if (opposite && opposite->vchild) return opposite->vchild;
    HbrMesh<T>* mesh = GetMesh();
    vchild = mesh->GetSubdivision()->Subdivide(mesh, this);
    vchild->SetParent(this);
    return vchild;
}

template <class T>
void
HbrHalfedge<T>::GuaranteeNeighbor() {
    HbrMesh<T>* mesh = GetMesh();
    mesh->GetSubdivision()->GuaranteeNeighbor(mesh, this);
}

// Determines whether an edge is infinitely sharp as far as its
// facevarying data is concerned. Happens if the faces on both sides
// disagree on the facevarying data at either of the shared vertices
// on the edge.
template <class T>
bool
HbrHalfedge<T>::GetFVarInfiniteSharp(int datum) {

    // Check to see if already initialized
    int intindex = datum >> 4;
    int shift = (datum & 15) << 1;
    unsigned int *fvarinfsharp = getFVarInfSharp();
    unsigned int bits = (fvarinfsharp[intindex] >> shift) & 0x3;
    if (bits != 3) {
        assert (bits != 2);
        return bits ? true : false;
    }
    
    // If there is no face varying data it can't be infinitely sharp!
    const int fvarwidth = GetMesh()->GetTotalFVarWidth();
    if (!fvarwidth) {
        bits = ~(0x3 << shift);
	fvarinfsharp[intindex] &= bits;
	if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
	return false;
    }

    // If either incident face is missing, it's a geometric boundary
    // edge, and also a facevarying boundary edge
    HbrFace<T>* left = GetLeftFace(), *right = GetRightFace();
    if (!left || !right) {
        bits = ~(0x2 << shift);
	fvarinfsharp[intindex] &= bits;
	if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
	return true;
    }

    // Look for the indices on each face which correspond to the
    // origin and destination vertices of the edge
    int lorg = -1, ldst = -1, rorg = -1, rdst = -1, i, nv;
    HbrHalfedge<T>* e;
    e = left->GetFirstEdge();
    nv = left->GetNumVertices();
    for (i = 0; i < nv; ++i) {
	if (e->GetOrgVertex() == GetOrgVertex()) lorg = i;
	if (e->GetOrgVertex() == GetDestVertex()) ldst = i;
	e = e->GetNext();
    }
    e = right->GetFirstEdge();
    nv = right->GetNumVertices();
    for (i = 0; i < nv; ++i) {
	if (e->GetOrgVertex() == GetOrgVertex()) rorg = i;
	if (e->GetOrgVertex() == GetDestVertex()) rdst = i;
	e = e->GetNext();	
    }    
    assert(lorg >= 0 && ldst >= 0 && rorg >= 0 && rdst >= 0);
    // Compare the facevarying data to some tolerance
    const int startindex = GetMesh()->GetFVarIndices()[datum];
    const int width = GetMesh()->GetFVarWidths()[datum];
    if (!right->GetFVarData(rorg).Compare(left->GetFVarData(lorg), startindex, width, 0.001f) ||
        !right->GetFVarData(rdst).Compare(left->GetFVarData(ldst), startindex, width, 0.001f)) {
        bits = ~(0x2 << shift);
	fvarinfsharp[intindex] &= bits;
	if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
        return true;
    }

    bits = ~(0x3 << shift);
    fvarinfsharp[intindex] &= bits;
    if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
    return false;
}

template <class T>
bool
HbrHalfedge<T>::IsFVarInfiniteSharpAnywhere() {
    for (int i = 0; i < GetMesh()->GetFVarCount(); ++i) {
        if (GetFVarInfiniteSharp(i)) return true;
    }
    return false;
}

template <class T>
float
HbrHalfedge<T>::GetFVarSharpness(int datum, bool ignoreGeometry) {

    bool infsharp = GetFVarInfiniteSharp(datum);

    if (infsharp) return k_InfinitelySharp;

    if (!ignoreGeometry) {
	// If it's a geometrically sharp edge it's going to be a
	// facevarying sharp edge too
	if (sharpness > k_Smooth) {
	    return k_InfinitelySharp;
	}
    }
    return k_Smooth;
}
    

template <class T>
std::ostream&
operator<<(std::ostream& out, const HbrHalfedge<T>& edge) {
    if (edge.IsBoundary()) out << "boundary ";
    out << "edge connecting ";
    if (edge.GetOrgVertex())
	out << *edge.GetOrgVertex();
    else
	out << "(none)";
    out << " to ";
    if (edge.GetDestVertex()) {
	out << *edge.GetDestVertex();
    } else {
	out << "(none)";
    }
    return out;
}

// Sorts half edges by the relative ordering of the incident faces'
// paths.
template <class T>
class HbrHalfedgeCompare {
public:
    bool operator() (const HbrHalfedge<T>* a, HbrHalfedge<T>* b) const {
	return (a->GetFace()->GetPath() < b->GetFace()->GetPath());
    }
};

template <class T>
class HbrHalfedgeOperator {
public:
    virtual void operator() (HbrHalfedge<T> &edge) = 0;
    virtual ~HbrHalfedgeOperator() {}
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* HBRHALFEDGE_H */
