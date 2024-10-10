import {webf} from './webf';

// type FormDataEntryValue = File | string;
type FormDataPart=[key:string,value:any]
type BlobPart=string|File|Blob;
export interface FormData {
    new():FormData;
    append(name: string, value: BlobPart, fileName?: string): void;
    // this method name will be fixed to **delete** when MemberInstaller::InstallFunctions is called
    form_data_delete(name: string): void;
    get(name: string): BlobPart
    getAll(name: string): BlobPart[];
    has(name: string): boolean;
    set(name: string, value: BlobPart, fileName?: string): void;
    forEach(callbackfn: Function, thisArg?: any): void;
    keys():string[]
    values():BlobPart[]
    entries():FormDataPart[]
}
