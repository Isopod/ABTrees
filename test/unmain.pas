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

unit unMain;

{$IFDEF FPC}
{$mode delphi}
{$OPTIMIZATION CSE}
{$OPTIMIZATION ASMCSE}
{$ENDIF}   {$H+}

{.$DEFINE DEBUG_ABTREE}

interface

uses
  {Windows,} Classes, SysUtils, FileUtil, Forms, Controls, Graphics, Dialogs, StdCtrls,
  ExtCtrls, math, unABTree;

type

  { TDrawingABTree }

  TDrawingABTree = class(TABTree)
    procedure Draw(Canvas: TCanvas; Offset: TPoint);
  end;

  ///////////

  { TForm1 }

  TForm1 = class(TForm)
    bIns10: TButton;
    bIns11: TButton;
    bIns12: TButton;
    bIns13: TButton;
    bIns14: TButton;
    bIns15: TButton;
    bIns16: TButton;
    bIns17: TButton;
    bIns18: TButton;
    bIns19: TButton;
    bIns2: TButton;
    bIns20: TButton;
    bIns21: TButton;
    bIns22: TButton;
    bIns23: TButton;
    bIns24: TButton;
    bIns25: TButton;
    bIns26: TButton;
    bIns27: TButton;
    bIns28: TButton;
    bIns29: TButton;
    bIns3: TButton;
    bIns30: TButton;
    bIns31: TButton;
    bIns32: TButton;
    bIns33: TButton;
    bIns34: TButton;
    bIns35: TButton;
    bIns36: TButton;
    bIns37: TButton;
    bIns38: TButton;
    bIns39: TButton;
    bIns4: TButton;
    bIns40: TButton;
    bIns41: TButton;
    bIns42: TButton;
    bIns43: TButton;
    bIns44: TButton;
    bIns45: TButton;
    bIns46: TButton;
    bIns47: TButton;
    bIns48: TButton;
    bIns49: TButton;
    bIns5: TButton;
    bIns50: TButton;
    bIns51: TButton;
    bIns52: TButton;
    bIns53: TButton;
    bIns54: TButton;
    bIns6: TButton;
    bIns7: TButton;
    bIns8: TButton;
    bIns9: TButton;
    bIns1: TButton;
    Button1: TButton;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Button5: TButton;
    Button6: TButton;
    Button7: TButton;
    Button8: TButton;
    Button9: TButton;
    Memo1: TMemo;
    Panel1: TPanel;
    Panel2: TPanel;
    procedure bIns1Click(Sender: TObject);
    procedure bRem1Click(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure Button8Click(Sender: TObject);
    procedure Button9Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure FormMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure FormMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
    procedure FormPaint(Sender: TObject);
    procedure FormResize(Sender: TObject);
  private
    { private declarations }
  public
    { public declarations }
    Tree: TDrawingABTree;
    DrawOffset: TPoint;
    LastPoint: TPoint;
    Buffer: TBitmap;
    procedure DrawTree;
  end;

var
  Form1: TForm1;

implementation

{$R *.lfm}


{ TDrawingABTree }


procedure TDrawingABTree.Draw(Canvas: TCanvas; Offset: TPoint);
var
  i,j,k,p,C: integer;
  s: string;
  Layers: array of array of PABNode;
  LayerNodeStart: array of array of single;
  LayerNodeParentIndex: array of array of integer;
  LayerNodeChildIndex: array of array of integer;
  x1,y1,x2,y2: integer;
  y, x: integer;
const
  LAYER_HEIGHT = 40;
  LEAF_WIDTH = 28;
  LEAF_WIDTH_BRUTTO = 30;

  function GetTreeHeight: integer;
  var
    Node: PABNode;
  begin
    Result := 2;
    Node := PABNode(Root);
    while (PABNavNode(Node).ChildCount > 0) and (not PABNavNode(Node).Children[0].IsLeaf) do
    begin
      Node := PABNode(PABNavNode(Node).Children[0]);
      inc(Result);
    end;
  end;

  procedure BuildLayers;
  var
    i,j,k,c: integer;
    height: integer;
    LayerWidth: integer;
  begin
    height := GetTreeHeight;

    SetLength(layers, height);
    SetLength(layers[0], 1);
    Layers[0][0] := PABNode(Root);

    LayerWidth := Root.ChildCount;
    for i := 1 to height - 1 do
    begin
      SetLength(layers[i], LayerWidth);
      LayerWidth := 0;
      c := 0;
      for j := low(layers[i-1]) to high(layers[i-1]) do
      begin
        for k := 0 to PABNavNode(layers[i-1][j]).ChildCount - 1 do
        begin
          layers[i][c] := PABNavNode(layers[i-1][j]).Children[k];
          inc(c);
          if i < height - 1 then
            LayerWidth := LayerWidth + PABNavNode(PABNavNode(layers[i-1][j]).Children[k]).ChildCount;
        end;
      end;
    end;
  end;

  procedure BuildParentIndex;
  var
    i,j,k,c: integer;
  begin
    SetLength(LayerNodeParentIndex, length(Layers));
    for i := low(layers) to high(layers) do
      SetLength(LayerNodeParentIndex[i], length(Layers[i]));

    for i := low(layers) to high(layers) - 1 do
    begin
      c := 0;
      for j := low(layers[i]) to high(layers[i]) do
      begin
        for k := 0 to PABNavNode(layers[i][j]).ChildCount - 1 do
        begin
          LayerNodeParentIndex[i+1][c] := j;
          inc(c);
        end;
      end;
    end;
  end;

  procedure BuildChildIndex;
  var
    i,j,k,c: integer;
  begin
    SetLength(LayerNodeChildIndex, length(Layers));
    for i := low(layers) to high(layers) do
      SetLength(LayerNodeChildIndex[i], length(Layers[i]));

    for i := low(layers) to high(layers) - 1 do
    begin
      c := 0;
      for j := low(layers[i]) to high(layers[i]) do
      begin
        for k := 0 to PABNavNode(layers[i][j]).ChildCount - 1 do
        begin
          LayerNodeChildIndex[i+1][c] := k;
          inc(c);
        end;
      end;
    end;
  end;

  procedure BuildLayerNodeStarts;
  var
    i,j,k: integer;
    PrevParentNode: PABNode;
  begin
    SetLength(LayerNodeStart, length(Layers));

    SetLength(LayerNodeStart[high(Layers)], length(Layers[high(Layers)]) + 1);
    for j := high(LayerNodeStart[high(Layers)]) downto low(LayerNodeStart[high(Layers)]) do
      LayerNodeStart[high(Layers)][j] := j * LEAF_WIDTH_BRUTTO;

    for i := high(LayerNodeStart) downto low(LayerNodeStart) + 1  do
    begin
      SetLength(LayerNodeStart[i-1], length(Layers[i-1]) + 1 );
      PrevParentNode := nil;
      k := 0;
      for j := low(LayerNodeStart[i]) to high(LayerNodeStart[i])  do
      begin
        if (j > high(Layers[i])) or (Layers[i][j].ParentNode <> PrevParentNode) then
        begin
          LayerNodeStart[i-1][k] := LayerNodeStart[i][j];
          inc(k);
        end;
        if (j < high(Layers[i])) then
          PrevParentNode := Layers[i][j].ParentNode;
      end;
    end;
  end;

begin
  BuildLayers;
  BuildParentIndex;
  BuildChildIndex;
  BuildLayerNodeStarts;
  for i := high(Layers) downto low(Layers) do
  begin
    y := i * LAYER_HEIGHT;
    for j := low(Layers[i]) to high(Layers[i]) do
    begin
      x1 := round((LayerNodeStart[i][j] + LayerNodeStart[i][j+1])/2);

      if i < high(Layers) then
      begin
        C := PABNavNode(Layers[i][j]).ChildCount;
        Canvas.Rectangle(
          x1 - (LEAF_WIDTH*C) div 2 + Offset.X,
          y + Offset.Y - 9,
          x1 + (LEAF_WIDTH*C) div 2 + Offset.X,
          y + Offset.Y + 9
        );
        for k := 0 to C-1 do
          Canvas.TextOut(
            x1 - (LEAF_WIDTH*C) div 2 + 5 + Offset.X + k*LEAF_WIDTH,
            y + Offset.Y + 1 - 8,
            IntToStr(PABNavNode(Layers[i][j]).Children[k].Key)
          );
      end
      else
      begin
        Canvas.Rectangle(
          x1-LEAF_WIDTH div 2 + Offset.X,
          y + Offset.Y - 9,
          x1 + LEAF_WIDTH div 2 + Offset.X,
          y + Offset.Y + 9
        );
        Canvas.TextOut(
          x1 - LEAF_WIDTH div 2 + 3 + Offset.X,
          y+Offset.Y+1-8,
          IntToStr(Layers[i][j].Key)
        );
        Canvas.Rectangle(
          x1-LEAF_WIDTH div 2 + Offset.X,
          y + Offset.Y + 8,
          x1 + LEAF_WIDTH div 2 + Offset.X,
          y + Offset.Y + 8 +18
        );
        Canvas.TextOut(
          x1 - LEAF_WIDTH div 2 + 3 + Offset.X,
          y+Offset.Y+18-8,
          IntToStr(PABLeaf(Layers[i][j]).Data)
        );
      end;

      if i >= 1 then
      begin
        p := LayerNodeParentIndex[i,j];
        C := PABNavNode(Layers[i-1][p]).ChildCount;
        x2 := round((LayerNodeStart[i-1][p] + LayerNodeStart[i-1][p+1])/2);
        Canvas.MoveTo(x1 + Offset.X, y+Offset.Y - 9);
        Canvas.LineTo(
          x2 + Offset.X - (LEAF_WIDTH * C) div 2  +  LayerNodeChildIndex[i,j] * LEAF_WIDTH + LEAF_WIDTH div 2,
          y  -LAYER_HEIGHT + Offset.Y + 9
        );
      end
    end;
  end;
end;

{ TForm1 }

procedure TForm1.FormCreate(Sender: TObject);
begin
  randomize;
  Tree := TDrawingABTree.Create;
  DrawOffset := Point(10,20);
  Buffer := TBitmap.Create;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
  Tree.Free;
  Buffer.Free;
end;

procedure TForm1.FormMouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbLeft then
    LastPoint := Point(X,Y);
end;

procedure TForm1.FormMouseMove(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
begin
  if ssLeft in Shift then
  begin
    DrawOffset.X := DrawOffset.X + X - LastPoint.X;
    DrawOffset.Y := DrawOffset.Y + Y - LastPoint.Y;
    LastPoint := Point(X,Y);
    DrawTree;
  end;
end;

procedure TForm1.FormPaint(Sender: TObject);
begin
  DrawTree;
end;

procedure TForm1.FormResize(Sender: TObject);
begin
  Buffer.SetSize(ClientWidth, ClientHeight);
end;

procedure TForm1.DrawTree;
begin
  Buffer.Canvas.FillRect(ClientRect);
  Tree.Draw(Buffer.Canvas, DrawOffset);
  {BitBlt(
    Canvas.Handle,
    0,0,ClientWidth, ClientHeight,
    Buffer.Canvas.Handle,
    0,0,
    srccopy);}
  Canvas.Draw(0,0,Buffer);
end;

procedure TForm1.Button1Click(Sender: TObject);
var
  i: integer;
begin
  for i := 0 to 100 do
    Tree.Insert(i, -i);
  DrawTree;
end;

procedure TForm1.Button2Click(Sender: TObject);
var
  i: integer;
begin
  for i := 0 to 71 do
    Tree.Remove(i);
  DrawTree;
end;

procedure TForm1.Button3Click(Sender: TObject);
begin
  Tree.Remove(72);
  DrawTree;
end;

procedure TForm1.Button4Click(Sender: TObject);
var
  i: integer;
  x: integer;
begin
  for i := 0 to 50 do
  begin
    x := random(100);
    Tree.Insert(x, -x);
  end;
  DrawTree;
end;

procedure TForm1.Button5Click(Sender: TObject);
var
  i: integer;
begin
  for i := 0 to 50 do
    Tree.Remove(random(100));
  DrawTree;
end;

procedure TForm1.Button6Click(Sender: TObject);
var
  Iterator: TABIterator;
begin
  Iterator := Tree.First;
  while not Iterator.IsNull do
  begin
    Memo1.Lines.Add(Format('%d: %d', [Iterator.Key, Iterator.Data]));
    Iterator.MoveNext;
  end;
  Memo1.Lines.Add('');
end;

procedure TForm1.Button7Click(Sender: TObject);
var
  Iterator: TABIterator;
begin
  Iterator := Tree.Last;
  while not Iterator.IsNull do
  begin
    Memo1.Lines.Add(Format('%d: %d', [Iterator.Key, Iterator.Data]));
    Iterator.MovePrevious;
  end;
  Memo1.Lines.Add('');
end;

procedure TForm1.Button8Click(Sender: TObject);
var
  Iterator: TABIterator;
  x: integer;
begin
  x := random(100);
  Iterator := Tree.Seek(x);
  if (not Iterator.IsNull) and (Iterator.Key = x) then
  begin
    if not Iterator.Previous.IsNull then
      Memo1.Lines.Add(Format('%d: %d', [Iterator.Previous.Key, Iterator.Previous.Data]));
    Memo1.Lines.Add(Format('%d: %d <--', [Iterator.Key, Iterator.Data]));
    if not Iterator.Next.IsNull then
      Memo1.Lines.Add(Format('%d: %d', [Iterator.Next.Key, Iterator.Next.Data]));
  end
  else
  begin
    Memo1.Lines.Add(Format('%d: not found', [x]));
  end;
  Memo1.Lines.Add('');
end;

procedure TForm1.Button9Click(Sender: TObject);
var
  Iterator: TABIterator;
begin
  Iterator := Tree.Seek(40);
  while (not Iterator.IsNull) and (Iterator.Key <= 60) do
  begin
    Iterator.Data := 1;
    Iterator.MoveNext;
  end;

  DrawTree;
end;

procedure TForm1.bIns1Click(Sender: TObject);
var
  k: integer;
begin
  k := StrToInt((Sender as TButton).Caption);
  Tree.Insert(k, -k);
  DrawTree;
end;

procedure TForm1.bRem1Click(Sender: TObject);
var
  k: integer;
begin
  k := StrToInt((Sender as TButton).Caption);
  Tree.Remove(k);
  DrawTree;
end;


end.

