## Shared pointers

Do not keep `ImageRef` or any kind of smart pointer to `doc::`
entities. As several `cmd` can persist in parallel with other `cmd`
(due the tree structure of the [undo history](../../undo/undo_history.h))
these smart pointers can generate conflicts in the logic layer.
E.g. If we keep an `ImageRef` inside a `cmd`, the image is
not removed from the [objects hash table](../../doc/object.cpp),
so two or more `cmd` could try to add/remove the same object
in the hash table (there are asserts to check this state, were
someone is trying to add the same `ObjectId` in the hash table).
