========================================================================
   d2cc -- a distributed compilation system
========================================================================

d2cc is a distributed compilation solution alternative to distcc_
and icecream_. It's still in early stages of development but the major
planned features are:

1. centralised scheduling of local tasks --- respecting max jobs
   per host independently of how many local and remote builds are
   running),

2. full zeroconf network support --- you just set the max number of jobs
   and start the daemon, it will autodiscover other d2cc instances
   and use them,

3. dynamic reconfiguration --- you can reconfigure, disable or enable
   the node at any point in time, without having to restart remote
   builds,

4. modular build --- making it easy to add support for new toolchains
   and schedulers.

.. _distcc: https://distcc.googlecode.com/
.. _icecream: https://github.com/icecc/icecream

.. vim:se ft=rst :
