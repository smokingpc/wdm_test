#pragma once

#define SHA1_IDENTIFIER 0x8004
#define SHA256_IDENTIFIER 0x800C
#define IMAGE_DIRECTORY_ENTRY_SECURITY  4

#if 0
// prepare the parameters required for calling CiCheckSignedFile
PolicyInfoGuard signerPolicyInfo;
PolicyInfoGuard timestampingAuthorityPolicyInfo;
LARGE_INTEGER signingTime = {};
//const int digestSize = 20; // sha1 len, 0x14
const  int digestSize = 32; // sha256 len, 0x20
//const int digestIdentifier = 0x8004; // sha1
const int digestIdentifier = 0x800C; // sha256
const BYTE digestBuffer[] = // 

{ 0x5f, 0x6d, 0xa4, 0x95, 0x78, 0xa4, 0x39, 0x4b, 0xb4, 0x0f,
  0xf6, 0x9b, 0xaa, 0x2a, 0xd7, 0x02, 0xda, 0x7d, 0x3d, 0xbe,
  0xb8, 0x12, 0xb8, 0xc7, 0x24, 0xcd, 0xe3, 0x68, 0x89, 0x65,
  0x86, 0x00 };
#endif

/**
*  This struct was copied from <wintrust.h> and encapsulates a signature used in verifying executable files.
*/
typedef struct _WIN_CERTIFICATE {
    ULONG  Length;                          // Specifies the length, in bytes, of the signature
    SHORT  Revision;                        // Specifies the certificate revision
    SHORT  CertType;                        // Specifies the type of certificate
    UCHAR  Certificate[ANYSIZE_ARRAY];      // An array of certificates
} WIN_CERTIFICATE, * LPWIN_CERTIFICATE;


/**
*  Describes the location (address) and size of a ASN.1 blob within a buffer.
*
*  @note  The data itself is not contained in the struct.
*/
typedef struct _ASN1_BLOBPTR
{
    int Size;               // size of the ASN.1 blob
    PVOID DataPtr;        // where the ASN.1 blob starts
} ASN1_BLOBPTR, * PASN1_BLOBPTR;


/**
*  Describes the location (address) and size of a certificate subject/issuer name, within a buffer.
*
*  @note  The data itself (name) is not contained in the struct.
*
*  @note  the reason for separating these fields into their own struct was to match the padding we
*         observed in CERT_CHAIN_MEMBER struct after the second 'short' field - once you enclose it
*         into a struct, on x64 bit machines there will be a padding of 4 bytes at the end of the struct,
*         because the largest member of the struct is of size 8 and it dictates the alignment of the struct.
*/
typedef struct _CERT_PARTY_NAME
{
    PVOID NamePtr;
    short NameLen;
    short Unknown;
} CERT_PARTY_NAME, * PCERT_PARTY_NAME;

/**
*  Contains various data about a specific certificate in the chain and also points to the actual certificate.
*
*  @note  the digest described in this struct is the digest that was used to create the certificate - not for
*         signing the file.
*
*  @note  The size reserved for digest is 64 byte regardless of the digest type, in order to accomodate SHA2/3's
*         max size of 512bit. The memory is not zeroed, so we must take the actual digestSize into account when
*         reading it.
*/
typedef struct _CERT_CHAIN_MEMBER
{
    int DigestIdetifier;                // e.g. 0x800c for SHA256
    int DigestSize;                     // e.g. 0x20 for SHA256
    UCHAR DigestBuffer[64];              // contains the digest itself, where the digest size is dictated by digestSize

    CERT_PARTY_NAME SubjectName;   // pointer to the subject name
    CERT_PARTY_NAME IssuerName;    // pointer to the issuer name

    ASN1_BLOBPTR Certificate;            // ptr to actual cert in ASN.1 - including the public key
} CERT_CHAIN_MEMBER, * PCERT_CHAIN_MEMBER;


/**
*  Describes the format of certChainInfo buffer member of PolicyInfo struct. This header maps the types,
*  locations, and quantities of the data which is contained in the buffer.
*
*  @note  when using this struct make sure to check its size first (bufferSize) because it's not guaranteed
*         that all the fields below will exist.
*/
typedef struct _CERT_CHAININFO_HEADER
{
    // The size of the dynamically allocated buffer
    int BufferSize;

    // points to the start of a series of Asn1Blobs which contain the public keys of the certificates in the chain
    PASN1_BLOBPTR PtrToPublicKeys;
    int NumberOfPublicKeys;

    // points to the start of a series of Asn1Blobs which contain the EKUs
    PASN1_BLOBPTR PtrToEkus;
    int NumberOfEkus;

    // points to the start of a series of CertChainMembers
    PCERT_CHAIN_MEMBER PtrToCertChainMembers;
    int NumberOfCertChainMembers;

    int Unknown;

    // ASN.1 blob of authenticated attributes - spcSpOpusInfo, contentType, etc.
    ASN1_BLOBPTR VariousAuthenticodeAttributes;
} CERT_CHAININFO_HEADER, * PCERT_CHAININFO_HEADER;


/**
*  Contains information regarding the certificates that were used for signing/timestamping
*
*  @note  you must check structSize before accessing the other members, since some members were added later.
*
*  @note  all structs members, including the length, are populated by ci functions - no need
*         to fill them in adavnce.
*/
typedef struct _POLICY_INFO
{
    int Size;
    NTSTATUS VerificationStatus;
    int flags;
    PCERT_CHAININFO_HEADER CertChain; // if not null - contains info about certificate chain
    FILETIME RevocationTime;            // when was the certificate revoked (if applicable)
    FILETIME ValidBeginTime;             // the certificate is not valid before this time
    FILETIME ExpireTime;              // the certificate is not valid before this time
} POLICY_INFO, * PPOLICY_INFO;

typedef PVOID(*CI_FREE_POLICY_INFO)(PPOLICY_INFO policyInfo);
typedef NTSTATUS(*CI_VALIDATE_FILE_OBJECT) (
    struct _FILE_OBJECT* fileObject,
    int a2,
    int a3,
    PPOLICY_INFO policyInfoForSigner,
    PPOLICY_INFO policyInfoForTimestampingAuthority,
    LARGE_INTEGER* signingTime,
    UCHAR* digestBuffer,
    int* digestSize,
    int* digestIdentifier);

void PrintCertInfo(PFILE_OBJECT imgfile);
BOOLEAN IsProcessFileSignatureValid(
    PFILE_OBJECT imgfile,
    const BYTE digest[],
    UINT32 algorithm,
    UINT32 digest_size);
