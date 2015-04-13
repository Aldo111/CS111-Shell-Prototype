#!/bin/bash


tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test1c.sh <<'EOF'
# test for simple commands
echo this simple command should run, count the numbers consecutively to see if anything is wrong

echo see statement below; echo if you followed the above statement and see this, then sequence commands work;

(echo 1; echo 2);

# simple test to see if Lab 1B is working

if echo 3; then echo 4; else echo ERROR if statement is not working; fi

if
    echo 5
then
    echo 6
else
    echo ERROR cascading if statements are not working
fi


if echo 7; then echo 8
else
    echo ERROR mixed if statements are not working
fi

# testing if if statements work with errors

if -1; then echo ERROR you shouldnt see this; else 9; fi

if -5; then echo ERROR you shouldnt see this;fi

if (echo 10; -1); then echo ERROR you shouldnt see this; fi # this is a comment

(echo 11 | cat )

echo 12 | cat | cat | sort

(echo 17; echo 18; echo 13; echo 15; echo 14; echo 16) | sort -n

while
    -1499
do
    echo you should see this
done

until
    echo 19
do
    echo shouldnt see this either
done

while -315413; do echo  this should be hidden too;
done

until echo 20    ; do echo this should also be hidden;  done;

while ( echo 21; -3251; echo 22; -3259);do
    echo nope, still shouldnt see this; done


# this is a comment

# another comment


if (echo 23; echo 24);
then
   if echo 25; then echo 26; else echo nope, dont look at this; fi
   echo 27; echo 28
   else
       echo this is wrong too
fi

if (
    if echo 29
    then
	echo 30
    fi;
    echo 31
    )
then
    echo 32
else
    echo not visible
fi

((echo A | echo B)|( echo C | echo D)) | ((echo E | echo F)|( echo G | echo 33)) | sort -n

exec echo 34

echo you should see up to 34
EOF

cat >test1c.exp <<'EOF'
this simple command should run, count the numbers consecutively to see if anything is wrong
see statement below
if you followed the above statement and see this, then sequence commands work
1
2
3
4
5
6
7
8
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
you should see up to 34
EOF

../profsh test1c.sh > test1c.out 2>test1c.err


diff -u test1c.exp test1c.out || exit

) || exit

rm -fr "$tmp"
