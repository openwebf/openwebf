interface SyncIterator {
  new(): void;
  next(): any;
  readonly [Symbol.iterator]: () => SyncIterator;
}