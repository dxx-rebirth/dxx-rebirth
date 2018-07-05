This branch contains the Git commits created from the Subversion history of the D2X snapshot used as the base for D2X-Rebirth.  To attach this history to the D2X-Rebirth history, fetch this branch, then `git replace --graft b94413b91b0a66cbc1d53054afc268bf0962d752 5288754d8952a9abade5ee908c9c5aa04e45b1e9`.  This will enable `git blame`, `git log`, and similar tools to trace from D2X-Rebirth history into D2X history.

For full history of D2X, including activity after the snapshot used for D2X-Rebirth, see [btb/d2x.git](https://github.com/btb/d2x).
