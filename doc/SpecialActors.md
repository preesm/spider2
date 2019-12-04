Spider 2.0: Special actors classification
=================

Abstract: Preesm and Spider 2.0 offers special actors as "syntaxic sugar" to help developers
manipulate data transfer in a PiSDF graph. The semantic of these actors is not necessarly the
same between Preesm and Spider 2.0. However, the automatic codegeneration of Preesm to Spider 2.0 
will convert the special actors of Preesm into the special actors of Spider 2.0.
This document lists the special actors of Spider 2.0 and their expected runtime and memory behavior.


#### Fork

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	fork [label="Fork Actor|{<i1> in||{<o1> o1|<o2> o2|...|<on> on}}" shape=Mrecord];
	input -> fork:i1[headlabel = "ri   "];
    
	fork:o1 -> output1[taillabel = "  ro1"];
    fork:o2 -> output2[taillabel = "  ro2"];
    fork:on -> outputn[taillabel = "  ron"];
}
```

*    1 input
*    n outputs
*    rates: in_rate == sum(out_rates)
*    memory behavior: input tokens are split in order of the output ports and w.r.t output rates
*    input is READ_ONLY
*    outputs are WRITE_ONLY

#### Join

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	join [label="Join Actor|{{<i1> i1|<i2> i2|...|<in> in}||<out> out}" shape=Mrecord];
	input1 -> join:i1[headlabel = "ri1   "];
	input2 -> join:i2[headlabel = "ri2   "];
	inputn -> join:in[headlabel = "rin   "];
    
	join:out -> output[taillabel = "  ro"];
}
```

*    n inputs
*    1 output
*    rates: out_rate == sum(in_rates)
*    memory behavior: input tokens are merged in order of the input ports and w.r.t input rates
*    inputs are READ_ONLY
*    output is WRITE_ONLY

#### Duplicate

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	Duplicate [label="Duplicate Actor|{<i1> in||{<o1> o1|<o2> o2|...|<on> on}}" shape=Mrecord];
	input -> Duplicate:i1[headlabel = "ri   "];
    
	Duplicate:o1 -> output1[taillabel = "  ri"];
    Duplicate:o2 -> output2[taillabel = "  ri"];
    Duplicate:on -> outputn[taillabel = "  ri"];
}
```

*    1 input
*    n outputs
*    rates: in_rate == out_rate, for every output
*    memory behavior: input tokens are duplicated on each output.
*    input is READ_ONLY
*    outputs are WRITE_ONLY

#### Tail

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	tail [label="Tail Actor|{{<i1> i1|<i2> i2|...|<in> in}||<out> out}" shape=Mrecord];
	input1 -> tail:i1[headlabel = "ri1   "];
	input2 -> tail:i2[headlabel = "ri2   "];
	inputn -> tail:in[headlabel = "rin   "];
    
	tail:out -> output[taillabel = "  ro"];
}
```

*    n inputs
*    1 output
*    rates: out_rate <= sum(in_rates)
*    memory behavior: output the last ro tokens produced on its input in order of production.
*    inputs are READ_ONLY
*    output is WRITE_ONLY

#### Head

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	head [label="Head Actor|{{<i1> i1|<i2> i2|...|<in> in}||<out> out}" shape=Mrecord];
	input1 -> head:i1[headlabel = "ri1   "];
	input2 -> head:i2[headlabel = "ri2   "];
	inputn -> head:in[headlabel = "rin   "];
    
	head:out -> output[taillabel = "  ro"];
}
```

*    n inputs
*    1 output
*    rates: out_rate <= sum(in_rates)
*    memory behavior: output the first ro tokens produced on its input in order of production.
*    inputs are READ_ONLY
*    output is WRITE_ONLY


#### Repeat

```graphviz
digraph structs {
	node[shape=record]
    rankdir=LR;
	repeat [label="Upsample Actor|{{<i1> i1|<i2> i2|...|<in> in}||<out> out}" shape=Mrecord];
	input1 -> repeat:i1[headlabel = "ri1   "];
	input2 -> repeat:i2[headlabel = "ri2   "];
	inputn -> repeat:in[headlabel = "rin   "];
    
	upsample:out -> output[taillabel = "  ro"];
}
```

*    1 input
*    1 output
*    rates: out_rate >= in_rate
*    memory behavior: Repeat the input tokens on its output in the order of production.
*    input is READ_ONLY
*    output is WRITE_ONLY

