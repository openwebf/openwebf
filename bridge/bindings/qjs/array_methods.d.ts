// @ts-ignore
@Mixin()
export interface ArrayLikeMethodsMixin {
  forEach(fn: Function, this_val?: any): void;
  entries(): SyncIterator;
  values(): SyncIterator;
  keys(): SyncIterator;
}