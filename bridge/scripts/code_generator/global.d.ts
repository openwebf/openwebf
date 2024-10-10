declare type int64 = number;
declare type double = number;
declare type JSArrayProtoMethod = void;

declare interface Dictionary {}

declare interface BlobPart {}
declare interface BlobPropertyBag {}
declare function Dictionary() : any;
declare type JSEventListener = void;

declare type LegacyNullToEmptyString = string | null;

// This property is implemented by Dart side
type DartImpl<T> = T;
type StaticMember<T> = T;
type ImplementedAs<T, S> = T;


type DependentsOnLayout<T> = T;