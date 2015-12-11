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

{ TABIterator }

function TABIterator.IsNull: Boolean;
begin
  Result := Node = nil;
end;

function TABIterator.Next: TABIterator;
begin
  // Nodes are ordered in reverse order internally
  if Assigned(Node) then
    Result.Node := Node.{Next}Prev
  else
    Result.Node := nil;
end;

function TABIterator.GetKey: TABKey;
begin
  ASSERT ( not IsNull, 'Tried to get key of non-existing node' );
  Result := Node.Key;
end;

function TABIterator.GetData: TABData;
begin
  ASSERT ( not IsNull, 'Tried to get data of non-existing node' );
  Result := Node.Data;
end;

procedure TABIterator.SetData(const NewData: TABData);
begin
  ASSERT ( not IsNull,  'Tried to set data of non-existing node' );
  Node.Data := NewData;
end;

procedure TABIterator.MoveNext;
begin
  Self := Next;
end;

function TABIterator.Previous: TABIterator;
begin
  // Nodes are ordered in reverse order internally
  if Assigned(Node) then
    Result.Node := Node.{Prev}Next
  else
    Result.Node := nil;
end;

procedure TABIterator.MovePrevious;
begin
  Self := Previous;
end;


{ TABLeaf }

{$IFDEF DEBUG_ABTREE}
procedure TABLeaf.AssertListInvariant;
begin
  ASSERT ( (Next = nil) or (Next.Prev = @Self), 'List invariant violated' );
  ASSERT ( (Prev = nil) or (Prev.Next = @Self), 'List invariant violated' );
end;
{$ENDIF}

procedure TABLeaf.InsertIntoLinkedList;
var
  Left,
  Right: PABNode;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertListInvariant;
  {$ENDIF}

  Left := Header.GetLeftCousin;
  Right := Header.GetRightCousin;

  ASSERT ( (Left = nil) or (Left.IsLeaf),
           'Cousin of leaf must be another leaf' );
  ASSERT ( (Right = nil) or (Right.IsLeaf),
           'Cousin of leaf must be another leaf' );

  Prev := PABLeaf(Left);
  Next := PABLeaf(Right);

  if Assigned(Prev) then
    Prev.Next := @Self
  else
    PABNavNode(Header.ParentNode).ParentTree.FirstLeaf := @Self;

  if Assigned(Next) then
    Next.Prev := @Self
  else
    PABNavNode(Header.ParentNode).ParentTree.LastLeaf := @Self;

  {$IFDEF DEBUG_ABTREE}
  AssertListInvariant;
  {$ENDIF}
end;

procedure TABLeaf.RemoveFromLinkedList;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertListInvariant;
  {$ENDIF}

  if Assigned(Prev) then
    Prev.Next := Self.Next
  else
    PABNavNode(Header.ParentNode).ParentTree.FirstLeaf := Self.Next;

  if Assigned(Next) then
    Next.Prev := Self.Prev
  else
    PABNavNode(Header.ParentNode).ParentTree.LastLeaf := Self.Prev;
end;

class function TABLeaf.Create(Key: TABKey; Data: TABData): PABLeaf;
begin
  New(Result);
  Result.Header.IsLeaf := True;
  Result.Key := Key;
  Result.Next := nil;
  Result.Prev := nil;
  Result.Data := Data;
end;

{ TABTree }

constructor TABTree.Create;
begin
  Root := TABNavNode.Create(Self);
end;

destructor TABTree.Destroy;
begin
  FreeSubTree(PABNode(Root));
  inherited;
end;

{$IFDEF DEBUG_ABTREE}
procedure TABTree.AssertSorted;
var
  PreviousKey: TABKey;
  IsFirst: Boolean;
  ListNode: PABLeaf;
  procedure Traverse(Node: PABNavNode; var PreviousKey: TABKey; var IsFirst: Boolean);
  var
    i: integer;
  begin
    for i := 0 to Node.ChildCount - 1 do
    begin
      if Node.Children[i].IsLeaf then
      begin
        ASSERT ( PABNode(ListNode) = Node.Children[i],
                 'List in wrong order' );
        ASSERT ( (ListNode.Next = nil) or (ListNode.Next.Prev = ListNode),
                 'List invariant violated' );
        ListNode := ListNode.Next;
        if IsFirst then
        begin
          PreviousKey := Node.Children[i].Key;
          IsFirst := False;
        end
        else
        begin
          ASSERT ( Node.Children[i].Key <= PreviousKey );
          PreviousKey := Node.Children[i].Key;
        end;
      end
      else if not Node.Children[i].IsLeaf then
        Traverse(PABNavNode(Node.Children[i]), PreviousKey, IsFirst);
    end;
  end;
begin
  ListNode := FirstLeaf;

  ASSERT ( (FirstLeaf = nil) or (FirstLeaf.Prev = nil) );
  ASSERT ( (LastLeaf = nil) or (LastLeaf.Next = nil) );

  IsFirst := True;
  Traverse(Root, PreviousKey, IsFirst);
end;

procedure TABTree.AssertUnique;
var
  Map: array[Byte] of boolean;
  i: integer;
  procedure Traverse(Node: PABNavNode);
  var
    i: integer;
  begin
    for i := 0 to Node.ChildCount - 1 do
    begin
      if Node.Children[i].IsLeaf and (Node.Children[i].Key <= 255) then
      begin
        ASSERT ( not Map[Node.Children[i].Key], 'Leaves must be unique' );
        Map[Node.Children[i].Key] := True;
      end
      else if not Node.Children[i].IsLeaf then
        Traverse(PABNavNode(Node.Children[i]));
    end;
  end;
begin
  for i := low(Map) to high(Map) do
    Map[i] := False;
  Traverse(Root);
end;
{$ENDIF}

procedure TABTree.Insert(Key: TABKey; Data: TABData);
var
  Leaf: PABLeaf;
  Node: PABNavNode;
  Index: integer;
begin
  Node := LocateParent(Key, Index);

  if (Index < 0) or (Node.Splitters[Index] <> Key) then
  begin
    Leaf := TABLeaf.Create(Key, Data);
    Node.InsertLocally(PABNode(Leaf));
    Leaf.InsertIntoLinkedList;
  end;

  {$IFDEF DEBUG_ABTREE}
  AssertSorted;
  AssertUnique;
  {$ENDIF}
end;

function TABTree.Seek(Key: TABKey): TABIterator;
var
  Index: integer;
  Node: PABNavNode;
begin
  Node := LocateParent(Key, Index);

  if (Index >= 0) and (Index < Node.ChildCount) then
    Result.Node := PABLeaf(Node.Children[Index])
  else
    Result.Node := nil;
end;

function TABTree.First: TABIterator;
begin
  // nodes are in reverse order internally
  Result.Node := {FirstLeaf}LastLeaf;
end;

function TABTree.Last: TABIterator;
begin
  // nodes are in reverse order internally
  Result.Node := {LastLeaf}FirstLeaf;
end;

function TABTree.LocateParent(Key: TABKey; out Index: integer): PABNavNode;
var
  Next: PABNode;
begin
  Next := PABNode(Root);
  repeat
    Result := PABNavNode(Next);
    Index := Result.LocateLocally(Key);
    if Index < 0 then Index := 0;
    Next := PABNavNode(Result).Children[Index];
  until (Index >= Result.ChildCount) or Next.IsLeaf;
end;

procedure TABTree.FreeSubTree(const Root: PABNode);
var
  i: integer;
begin
  if Root.IsLeaf then
    Root.Free
  else
  begin
    for i := 0 to PABNavNode(Root).ChildCount - 1 do
      FreeSubTree(PABNavNode(Root).Children[i]);
    Root.Free;
  end;
end;


procedure TABTree.Remove(Key: TABKey);
var
  Node: PABNavNode;
  Index: integer;
begin
  Node := LocateParent(Key, Index);

  if (Index >= 0) and (Index < Node.ChildCount) and (Node.Splitters[Index] = key) then
  begin
    PABLeaf(Node.Children[Index]).RemoveFromLinkedList;
    Node.RemoveLocally(Index);
  end
  else
    { tried to remove non-existing key };

  {$IFDEF DEBUG_ABTREE}
  AssertSorted;
  AssertUnique;
  {$ENDIF}
end;

{ TABNode }

function TABNode.IsRoot: Boolean;
begin
  Result := ParentNode = nil;
end;

type
  PABNodeHeaderWithKey = ^TABNodeHeaderWithKey;
  TABNodeHeaderWithKey = record
    Header: TABNode;
    Key: TABKey;
  end;
function TABNode.Key: TABKey;
begin
  Result := PABNodeHeaderWithKey(@Self).Key;
end;

procedure TABNode.Free;
var
  _NavNode: PABNavNode;
  _Leaf: PABLeaf;
begin
  if IsLeaf then
  begin
    _Leaf := @Self;
    Dispose(_Leaf);
  end
  else
  begin
    _NavNode := @Self;
    Dispose(_NavNode);
  end;
end;

{$IFDEF DEBUG_ABTREE}
procedure TABNode.AssertKeyInvariant;
begin
  if Assigned(ParentNode) then
  begin
    ASSERT( PABNavNode(ParentNode).Splitters[Index] = Self.Key,
            'Value in splitter must correspond to the lowest "right" key in the ' +
            'subtree' );
    PABNode(ParentNode).AssertKeyInvariant;
  end;
end;
{$ENDIF}


function TABNode.GetLeftSibling: PABNode;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertKeyInvariant;
  {$ENDIF}
  if Index >= 1 then
    Result := PABNode(PABNavNode(ParentNode).Children[Index - 1])
  else
    Result := nil;
end;

function TABNode.GetRightSibling: PABNode;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertKeyInvariant;
  {$ENDIF}
  if Index < PABNavNode(ParentNode).ChildCount - 1 then
    Result := PABNode(PABNavNode(ParentNode).Children[Index + 1])
  else
    Result := nil;
end;

function TABNode.GetLeftCousin: PABNode;
var
  Uncle: PABNavNode;
begin
  if not IsRoot then
  begin
    if Index >= 1 then
      Result := PABNavNode(ParentNode).Children[Index - 1]
    else
    begin
      Uncle := PABNavNode(ParentNode.GetLeftCousin);
      if Assigned(Uncle) then
        Result := Uncle.Children[Uncle.ChildCount - 1]
      else
        Result := nil;
    end;
  end
  else { if IsRoot then }
    Result := nil;
end;

function TABNode.GetRightCousin: PABNode;
var
  Uncle: PABNavNode;
begin
  if not IsRoot then
  begin
    if Index <= PABNavNode(ParentNode).ChildCount - 2 then
      Result := PABNavNode(ParentNode).Children[Index + 1]
    else
    begin
      Uncle := PABNavNode(ParentNode.GetRightCousin);
      if Assigned(Uncle) then
        Result := Uncle.Children[0]
      else
        Result := nil;
    end;
  end
  else { if IsRoot then }
    Result := nil;
end;

{ TABNavNode }

procedure TABNavNode.SwapChildren(Index1, Index2: integer);
var
  TmpKey: TABKey;
  TmpChild: PABNode;
begin
  TmpKey := Splitters[Index1];
  TmpChild := Children[Index1];

  Splitters[Index1] := Splitters[Index2];
  Children[Index1] := Children[Index2];
  Children[Index1].Index := Index1;

  Splitters[Index2] := TmpKey;
  Children[Index2] := TmpChild;
  Children[Index2].Index := Index2;
end;

procedure TABNavNode.UpdateKey(Index: integer; NewKey: TABKey);
begin
  Splitters[Index] := NewKey;
  while (Index > 0) and (Splitters[Index - 1] < Splitters[Index]) do
  begin
    SwapChildren(Index - 1, Index);
    dec(Index);
  end;

  while (Index < ChildCount - 1) and (Splitters[Index + 1] > Splitters[Index]) do
  begin
    SwapChildren(Index + 1, Index);
    inc(Index);
  end;

  RestoreKeyInvariant;
  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}
end;

procedure TABNavNode.Split(Rightmost: PABNode);
var
  NewSibling: PABNavNode;
  PivotPos: integer;
  i: integer;
begin
  ASSERT ( ChildCount = B,
           'split precondition: node must have maximum outdegree ( + 1 )');
  ASSERT ( Rightmost.Key <= Splitters[ChildCount - 1] );
  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}

  NewSibling := TABNavNode.Create(ParentTree);
  PivotPos := (B - 1) div 2 + 1;

  Move(Splitters[PivotPos], NewSibling.Splitters[0], SizeOf(Splitters[0]) * (B - PivotPos));
  Move(Children[PivotPos], NewSibling.Children[0], SizeOf(Children[0]) * (B - PivotPos));

  NewSibling.ChildCount := B - PivotPos + 1;
  ChildCount := PivotPos;

  ASSERT ( NewSibling.ChildCount + ChildCount = B + 1 );

  NewSibling.Children[NewSibling.ChildCount - 1] := Rightmost;
  NewSibling.Splitters[NewSibling.ChildCount - 1] := Rightmost.Key;

  for i := 0 to NewSibling.ChildCount - 1 do
    NewSibling.RestoreParentInvariant(i);

  NewSibling.RestoreKeyInvariant;
  RestoreKeyInvariant;

  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}

  if not Header.IsRoot then
  begin
    // Restore invariant that Key = rightmost leaf
    PABNavNode(Header.ParentNode).UpdateKey(Header.Index, PABNode(@Self).Key);
  end
  else {if Header.IsRoot then}
  begin
    // new root
    Header.ParentNode := PABNode(TABNavNode.Create(ParentTree));
    ParentTree.Root := PABNavNode(Header.ParentNode);
    PABNavNode(Header.ParentNode).InsertLocally(@Self);
  end;

  {$IFDEF DEBUG_ABTREE}
  NewSibling.AssertChildrenKeysSortedInvariant;
  {$ENDIF}

  ASSERT ( NewSibling.Children[0].Key <= Children[ChildCount-1].Key );
  ASSERT ( PABNode(NewSibling).Key <= PABNode(@Self).Key );

  // Insert right half in higher level
  PABNavNode(Header.ParentNode).InsertLocally(PABNode(NewSibling));

  {$IFDEF DEBUG_ABTREE}
  Header.AssertKeyInvariant;
  AssertOutdegreeInvariant;
  AssertParentInvariant;
  AssertChildrenKeysSortedInvariant;

  NewSibling.Header.AssertKeyInvariant;
  NewSibling.AssertOutdegreeInvariant;
  NewSibling.AssertParentInvariant;
  NewSibling.AssertChildrenKeysSortedInvariant;
  {$ENDIF}
end;

function TABNavNode.LocateLocally(Key: TABKey): integer;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}
  Result := ChildCount - 1;
  while (Result >= 0) and (Key > Splitters[Result]) do
    dec(Result);
end;


procedure TABNavNode.RestoreParentInvariant(ChildIndex: integer);
begin
  ASSERT ( (ChildIndex >= 0) and (ChildIndex < ChildCount),
           'Modifying non-existant/out-of-range child' );
  Children[ChildIndex].Index := ChildIndex;
  Children[ChildIndex].ParentNode := @Self;
end;

{$IFDEF DEBUG_ABTREE}
procedure TABNavNode.AssertOutdegreeInvariant;
begin
  ASSERT( ChildCount <= B,
          'Node may only have up to B children' );
  ASSERT( Header.IsRoot or (ChildCount >= A),
          'Nodes other than the root must have at least A children' );
end;

procedure TABNavNode.AssertParentInvariant;
var
  i: integer;
begin
  for i := 0 to ChildCount - 1 do
  begin
    ASSERT( Children[i].ParentNode = @Self,
            'Child''s parent pointer pointing to other node' );
    ASSERT( Children[i].Index = i,
            'Child''s index not matching actual splitter position' );
  end;
end;

procedure TABNavNode.AssertChildrenKeysSortedInvariant;
var
  i: integer;
begin
  for i := 1 to ChildCount - 1 do
  begin
    ASSERT( Splitters[i] <= Splitters[i - 1],
            'Splitters are not sorted');
  end;
end;
{$ENDIF}

procedure TABNavNode.InsertLocally(Node: PABNode);
var
  InsertPos: integer;
  i: integer;
  Rightmost: PABNode;
begin
  InsertPos := LocateLocally(Node.Key) + 1;

  ASSERT ( (InsertPos = ChildCount) or (Children[InsertPos].Key <> Node.Key),
           'Keys in tree must be unique' );

  if ChildCount <> B then
  begin
    // within capacity, proceed normally

    Move(Splitters[InsertPos], Splitters[InsertPos + 1], SizeOf(Splitters[0]) * (B - 1 - InsertPos));
    Move(Children[InsertPos], Children[InsertPos + 1], SizeOf(Children[0]) * (B - 1 - InsertPos));

    Splitters[InsertPos] := Node.Key;
    Children[InsertPos] := Node;

    inc(ChildCount);

    for i := ChildCount - 1 downto InsertPos do
      RestoreParentInvariant(i);

    RestoreKeyInvariant;
  end
  else { if ChildCount = B then }
  begin
    if InsertPos <> B then
    begin
      // over capacity, "normal" case
      Rightmost := Children[B - 1];

      Move(Splitters[InsertPos], Splitters[InsertPos + 1], SizeOf(Splitters[0]) * (B - 1 - InsertPos));
      Move(Children[InsertPos], Children[InsertPos + 1], SizeOf(Children[0]) * (B - 1 - InsertPos));

      Splitters[InsertPos] := Node.Key;
      Children[InsertPos] := Node;

      for i := B - 1 downto InsertPos do
        RestoreParentInvariant(i);

      Split(Rightmost);
    end
    else { if InsertPos = B then }
    begin
      // over capacity, but new node is rightmost -> easy
      Split(Node);
    end
  end;


  {$IFDEF DEBUG_ABTREE}
  Header.AssertKeyInvariant;
  AssertOutdegreeInvariant;
  AssertParentInvariant;
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}
end;

procedure TABNavNode.RemoveSplitter(Index: integer);
var
  i: integer;
begin
  ASSERT ( (Index >= 0) and (Index < ChildCount) );

  Move(Splitters[Index + 1], Splitters[Index], SizeOf(Splitters[0]) * (ChildCount {- 1} - Index));
  Move(Children[Index + 1], Children[Index], SizeOf(Children[0]) * (ChildCount {- 1} - Index));

  dec(ChildCount);
  for i := Index to ChildCount - 1 do
    RestoreParentInvariant(i);

  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  AssertParentInvariant;
  {$ENDIF}
end;


procedure TABNavNode.RestoreOutdegreeInvariant;
var
  LeftSibling, RightSibling, Sibling: PABNavNode;
begin
  {$IFDEF DEBUG_ABTREE}
  Header.AssertKeyInvariant;
  AssertParentInvariant;
  {$ENDIF}

  if (ChildCount < A) and not Header.IsRoot then
  begin
    // EITHER FUSE OR BALANCE

    LeftSibling := PABNavNode(Header.GetLeftSibling);
    RightSibling := PABNavNode(Header.GetRightSibling);

    ASSERT ( (LeftSibling <> nil) or (RightSibling <> nil),
             'Only root is allowed to have no siblings. Otherwise we have an ' +
             'invalid node with outdegree one' );

    if Assigned(LeftSibling) and Assigned(RightSibling) then
    begin
      // If we have a choice between two siblings, choose the "less full" sibling
      if LeftSibling.ChildCount <= RightSibling.ChildCount then
        Sibling := LeftSibling
      else
        Sibling := RightSibling;
    end
    else if Assigned(LeftSibling)  then
      Sibling := LeftSibling
    else {if Assigned(RightSibling)  then}
      Sibling := RightSibling;

    ASSERT ( Sibling.Header.ParentNode = Header.ParentNode,
             'Siblings must have same parent' );

    if (Sibling.ChildCount + ChildCount <= B) then
      Fuse(Sibling)
    else
    begin
      // If we arrive here, fuse could not be done! -> Balance
      if Assigned(LeftSibling) and Assigned(RightSibling) then
      begin
        // If we have a choice between two siblings, choose the "fuller" sibling
        if LeftSibling.ChildCount >= RightSibling.ChildCount then
          Sibling := LeftSibling
        else
          Sibling := RightSibling;
      end;
      Balance(Sibling);
    end;
  end
  else if Header.IsRoot and (ChildCount = 1) and (not Children[0].IsLeaf) then
  begin
    // if root is empty, REMOVE ROOT AND REPLACE WITH ONLY CHILD

    ParentTree.Root := PABNavNode(Children[0]);
    Children[0].ParentNode := nil; // IMPORTANT
    PABNode(@Self).Free;
  end;
end;

procedure TABNavNode.RemoveLocally(Index: integer);
begin
  Children[Index].Free;

  RemoveSplitter(Index);
  RestoreKeyInvariant;

  RestoreOutdegreeInvariant;
end;

procedure TABNavNode.RestoreKeyInvariant;
begin
  {$IFDEF DEBUG_ABTREE}
  AssertChildrenKeysSortedInvariant;
  {$ENDIF}
  if Assigned(Header.ParentNode) then
  begin
    if PABNavNode(Header.ParentNode).Splitters[Header.Index] <> PABNode(@Self).Key then
    begin
      PABNavNode(Header.ParentNode).Splitters[Header.Index] := PABNode(@Self).Key;
      PABNavNode(Header.ParentNode).RestoreKeyInvariant;
    end;
  end;
end;

class function TABNavNode.Create(ParentTree: TABTree): PABNavNode;
begin
  New(Result);
  Result.Header.IsLeaf := False;
  Result.Header.ParentNode := nil;
  Result.ParentTree := ParentTree;
  Result.ChildCount := 0;
end;

procedure TABNavNode.Fuse(Target: PABNavNode);
var
  i: integer;
begin
  ASSERT ( not Header.IsRoot and not Target.Header.IsRoot,
           'Can''t fuse (with) root' );

  PABNavNode(Header.ParentNode).RemoveSplitter(Header.Index);

  for i := 0 to ChildCount - 1 do
    Target.InsertLocally(Children[i]);

  Target.RestoreKeyInvariant;

  PABNavNode(Header.ParentNode).RestoreKeyInvariant;

  if Assigned(Target.Header.ParentNode) then
    PABNavNode(Target.Header.ParentNode).RestoreOutdegreeInvariant;

  {$IFDEF DEBUG_ABTREE}
  Target.Header.AssertKeyInvariant;
  Target.AssertOutdegreeInvariant;
  Target.AssertParentInvariant;
  Target.AssertChildrenKeysSortedInvariant;
  {$ENDIF}

  PABNode(@Self).Free;
end;

procedure TABNavNode.Balance(Target: PABNavNode);
var
  i: integer;
begin
  ASSERT ( not Header.IsRoot and not Target.Header.IsRoot,
           'Can''t balance (with) root' );
  ASSERT ( Header.ParentNode = Target.Header.ParentNode,
           'Can only balance between siblings' );
  ASSERT ( Target <> @Self,
           'Can''t balance with self' );

  if Target.Header.Index < Header.Index then
  begin
    for i := Target.ChildCount - 1 downto 0 do
    begin
      ASSERT ( Target.ChildCount >= A );

      InsertLocally(Target.Children[i]);
      Target.RemoveSplitter(i);

      ASSERT ( Target.ChildCount >= A );
      if ChildCount >= Target.ChildCount then break;
    end;
    Target.RestoreKeyInvariant;
    RestoreKeyInvariant;
  end
  else
  begin
    for i := 0 to Target.ChildCount - 1 do
    begin
      ASSERT ( Target.ChildCount >= A );

      InsertLocally(Target.Children[0]);
      Target.RemoveSplitter(0);

      ASSERT ( Target.ChildCount >= A );
      if ChildCount >= Target.ChildCount then break;
    end;
    Target.RestoreKeyInvariant;
    RestoreKeyInvariant;
  end;

  {$IFDEF DEBUG_ABTREE}
  Header.AssertKeyInvariant;
  AssertOutdegreeInvariant;
  AssertParentInvariant;
  AssertChildrenKeysSortedInvariant;

  Target.Header.AssertKeyInvariant;
  Target.AssertOutdegreeInvariant;
  Target.AssertParentInvariant;
  Target.AssertChildrenKeysSortedInvariant;
  {$ENDIF}
end;
