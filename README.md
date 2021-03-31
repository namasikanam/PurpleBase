# README 

This is a developing simple DBMS for course *Introduction to Database Management System* at Tsinghua University, based on [RedBase](https://web.stanford.edu/class/cs346/2015/purplebase.html). It's planned to follows both the specification of [CS346](https://cs346.stanford.edu/) and *Introduction to Database Management System*.

Purple is the color of Tsinghua and also was my favorite color when I was in middle school. So, I use it to name my DBMS.

## Instructions

* **Setup**: Install ``g++``, ``bison``, ``flex`` and ``valgrind``  
    * This project runs in 32-bit mode, so if your OS is 64-bit, you also need install `g++-multilib`.

* **Compile**: Run ``make`` to compile the code and ``make testers`` to compile the tests (Add the required test files to the ``Makefile``)


* **Run tests**: Run the tests under valgrind.
For example, ``$ valgrind ./rm_test``

## To-Do Extension

Here're some options to implement for extra credits. (Now they're all from the document of CS346).
- Let `IX` module be able to accommodate any number of `RID`s for an attribute value.
  - Maybe through bucket chaining.
- Implement fully correct (e.g. rebalancing) deletion of B+ tree.
  - Maybe throught the way specified by [this paper](https://web.stanford.edu/class/cs346/2015/notes/jannink.pdf).

## References

* [Aditya Bhandari's RedBase](https://github.com/adityabhandari1992/cs346-redbase) (adityasb@stanford.edu)
* [RedBase from Stanford CS346](https://web.stanford.edu/class/cs346/2015/redbase.html)
* [Zecong Hu and Yilong Wei's rebaseDB](https://github.com/huzecong/rebaseDB)
