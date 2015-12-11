{ Copyright (c) 2015 Philip Zander <philip.zander@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
}

type
  PABNode = ^TABNode;

  { TABNode }

  TABNode = record
    IsLeaf: Boolean;
    ParentNode: PABNode;
    Index: integer;
  private
    {$IFDEF DEBUG_ABTREE}
    procedure AssertKeyInvariant;
    {$ENDIF}
  private
    function GetLeftSibling: PABNode; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    function GetRightSibling: PABNode; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    function GetLeftCousin: PABNode; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    function GetRightCousin: PABNode; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    function IsRoot: Boolean; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  public
    function Key: TABKey; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure Free; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  end;

  TABSplitters = array[0..B-1] of TABKey;
  TABChildren = array[0..B-1] of PABNode;

  TABTree = class;

  { TABNavNode }

  PABNavNode = ^TABNavNode;
  TABNavNode = record
    // <DO NOT CHANGE>
    Header: TABNode;
    Splitters: TABSplitters;
    // </DO NOT CHANGE>
    ChildCount: integer;
    Children: TABChildren;
    ParentTree: TABTree;
  private
    procedure RestoreKeyInvariant; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure RestoreOutdegreeInvariant; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure RestoreParentInvariant(ChildIndex: integer); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}

    {$IFDEF DEBUG_ABTREE}
    procedure AssertOutdegreeInvariant;
    procedure AssertParentInvariant;
    procedure AssertChildrenKeysSortedInvariant;
    {$ENDIF}
  private
    procedure RemoveSplitter(Index: integer); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure SwapChildren(Index1, Index2: integer); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure UpdateKey(Index: integer; NewKey: TABKey); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  private
    procedure Split(Rightmost: PABNode); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure Fuse(Target: PABNavNode); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure Balance(Target: PABNavNode); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  public
    procedure InsertLocally(Node: PABNode); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure RemoveLocally(Index: integer); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    function LocateLocally(Key: TABKey): integer; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}

    class function Create(ParentTree: TABTree): PABNavNode; static; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  end;

  { TABLeaf }

  PABLeaf = ^TABLeaf;
  TABLeaf = record
    // <DO NOT CHANGE>
    Header: TABNode;
    Key: TABKey;
    // </DO NOT CHANGE>
    Next, Prev: PABLeaf;
    Data: TABData;
  private
    {$IFDEF DEBUG_ABTREE}
    procedure AssertListInvariant;
    {$ENDIF}
  public
    procedure InsertIntoLinkedList; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure RemoveFromLinkedList; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}

    class function Create(Key: TABKey; Data: TABData): PABLeaf; static; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  end;

  { TABIterator }

  TABIterator = record
  private
    Node: PABLeaf;
    function GetKey: TABKey;
    function GetData: TABData; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure SetData(const NewData: TABData); {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  public
    function IsNull: Boolean; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}

    property Key: TABKey read GetKey;
    property Data: TABData read GetData write SetData;

    function Next: TABIterator; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure MoveNext; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}

    function Previous: TABIterator; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
    procedure MovePrevious; {$IFDEF ABTREE_INLINE}inline;{$ENDIF}
  end;

  { TABTree }

  TABTree = class
  protected
    FirstLeaf, LastLeaf: PABLeaf;
    Root: PABNavNode;
    function LocateParent(Key: TABKey; out Index: integer): PABNavNode;
    procedure FreeSubTree(const Root: PABNode);
  protected
    {$IFDEF DEBUG_ABTREE}
    procedure AssertSorted;
    procedure AssertUnique;
    {$ENDIF}
  public
    constructor Create;
    destructor Destroy; override;

    function First: TABIterator;
    function Last: TABIterator;

    function Seek(Key: TABKey): TABIterator;
    procedure Insert(Key: TABKey; Data: TABData);
    procedure Remove(Key: TABKey);
  end;
