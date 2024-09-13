#include <wdm.h>
#include <minwindef.h>
#include "FileSignatureCheck.h"

//Roy Note:
//CiXXXX APIs in ci.dll is used for kernel authenticode and driver signature verify.
//ELAM also use it. This is only way to support digital signature checking without 
//additional librarys in windows kernel.


#if 0
/**
*  Given a file digest and signature of a file, verify the signature and provide information regarding
*  the certificates that was used for signing (the entire certificate chain)
*
*  @note  the function allocates a buffer from the paged pool --> can be used only where IRQL < DISPATCH_LEVEL
*
*  @param  digestBuffer - buffer containing the digest
*
*  @param  digestSize - size of the digest, e.g. 0x20 for SHA256, 0x14 for SHA1
*
*  @param  digestIdentifier - digest algorithm identifier, e.g. 0x800c for SHA256, 0x8004 for SHA1
*
*  @param  winCert - pointer to the start of the security directory
*
*  @param  sizeOfSecurityDirectory - size the security directory
*
*  @param  policyInfoForSigner[out] - PolicyInfo containing information about the signer certificate chain
*
*  @param  signingTime[out] - when the file was signed (FILETIME format)
*
*  @param  policyInfoForTimestampingAuthority[out] - PolicyInfo containing information about the timestamping
*          authority (TSA) certificate chain
*
*  @return  0 if the file digest in the signature matches the given digest and the signer cetificate is verified.
*           Various error values otherwise, for example:
*           STATUS_INVALID_IMAGE_HASH - the digest does not match the digest in the signature
*           STATUS_IMAGE_CERT_REVOKED - the certificate used for signing the file is revoked
*           STATUS_IMAGE_CERT_EXPIRED - the certificate used for signing the file has expired
*/
extern "C" __declspec(dllimport) NTSTATUS _stdcall CiCheckSignedFile(
    const PVOID digestBuffer,
    int digestSize,
    int digestIdentifier,
    const LPWIN_CERTIFICATE winCert,
    int sizeOfSecurityDirectory,
    PPOLICY_INFO policyInfoForSigner,
    LARGE_INTEGER * signingTime,
    PPOLICY_INFO policyInfoForTimestampingAuthority);
#endif

/**
*  Resets a PolicyInfo struct - frees the dynamically allocated buffer in PolicyInfo (certChainInfo) if not null.
*  Zeros the entire PolicyInfo struct.
*
*  @param  policyInfo - the struct to reset.
*
*  @return  the struct which was reset.
*/
//extern "C" __declspec(dllimport) PVOID _stdcall CiFreePolicyInfo(PPOLICY_INFO policyInfo);
extern CI_FREE_POLICY_INFO CiFreePolicyInfo;

/**
*  Given a file object, verify the signature and provide information regarding
*  the certificates that was used for signing (the entire certificate chain)
*
*  @note  the function allocates memory from the paged pool --> can be used only where IRQL < DISPATCH_LEVEL
*
*  @param  fileObject[in] - fileObject of the PE in question
*
*  @param  a2[in] - unknown, needs to be reversed. 0 is a valid value.
*
*  @param  a3[in] - unknown, needs to be reversed. 0 is a valid value.
*
*  @param  policyInfoForSigner[out] - PolicyInfo containing information about the signer certificate chain
*
*  @param  signingTime[out] - when the file was signed
*
*  @param  policyInfoForTimestampingAuthority[out] - PolicyInfo containing information about the timestamping
*          authority (TSA) certificate chain
*
*  @param  digestBuffer[out] - buffer to be filled with the digest, must be at least 64 bytes
*
*  @param  digestSize[inout] - size of the digest. Must be at leat 64 and will be changed by the function to
*                              reflect the actual digest length.
*
*  @param  digestIdentifier[out] - digest algorithm identifier, e.g. 0x800c for SHA256, 0x8004 for SHA1
*
*  @return  0 if the file digest in the signature matches the given digest and the signer cetificate is verified.
*           Various error values otherwise, for example:
*           STATUS_INVALID_IMAGE_HASH - the digest does not match the digest in the signature
*           STATUS_IMAGE_CERT_REVOKED - the certificate used for signing the file is revoked
*           STATUS_IMAGE_CERT_EXPIRED - the certificate used for signing the file has expired
*/
//extern "C" __declspec(dllimport) NTSTATUS _stdcall CiValidateFileObject(
//    struct _FILE_OBJECT* fileObject,
//    int a2,
//    int a3,
//    PPOLICY_INFO policyInfoForSigner,
//    PPOLICY_INFO policyInfoForTimestampingAuthority,
//    LARGE_INTEGER * signingTime,
//    UCHAR * digestBuffer,
//    int* digestSize,
//    int* digestIdentifier
//);
extern CI_VALIDATE_FILE_OBJECT CiValidateFileObject;

void PrintCertInfo(PFILE_OBJECT imgfile)
{
    POLICY_INFO signer_policy = { 0 };
    POLICY_INFO time_auth_policy = { 0 };   //timestamp authority policy
    LARGE_INTEGER sign_time = { 0 };
    int digest_size = 64;                    //must be greater than 64
    int digest_ident = 0;
    UCHAR digest_buffer[64] = { 0 };          //must be greater than 64
    NTSTATUS status = CiValidateFileObject(
        imgfile,
        0,
        0,
        &signer_policy,
        &time_auth_policy,
        &sign_time,
        digest_buffer,
        &digest_size,
        &digest_ident);

    KdPrint(("[Roy] CiValidateFileObject returned 0x%08X\n", status));
    if (NT_SUCCESS(status))
    {
        KdPrint(("[Roy] verification status:0x%08X, digest algorithm:0x%X, digest size:%d\n", 
            signer_policy.VerificationStatus, digest_ident, digest_size));
        KdPrint(("[Roy] digest algorithm:0x%X, digest size:%d\n", 
            digest_ident, digest_size));
        //KdPrint(("[Roy] digest algorithm:0x%X, digest size:%d, subject=%s, issuer=%s\n",
        //    digest_ident, digest_size, signer_policy.CertChain->PtrToCertChainMembers->SubjectName));
        KdPrint(("[Roy] digest buffer:"));

        for(int i=0; i< digest_size; i++)
        {
            KdPrint(("%02X", signer_policy.CertChain->PtrToCertChainMembers->DigestBuffer[i]));
        }

        KdPrint(("\n\n"));
    }
    CiFreePolicyInfo(&signer_policy);
    CiFreePolicyInfo(&time_auth_policy);
}

BOOLEAN ValidateFileUsingCiValidateFileObject(PFILE_OBJECT imgfile, const BYTE digest[], UINT digest_size)
{

    KdPrint(("[Roy] Validating file using CiValidateFileObject...\n"));
    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    POLICY_INFO signerPolicyInfo = { 0 };
    POLICY_INFO timestampingAuthorityPolicyInfo = { 0 };
    LARGE_INTEGER signingTime = { 0 };
    int digestSize = 64;  //must be greater than 64
    int digestIdentifier = 0;
    BYTE digestBuffer[64] = { 0 }; //must be greater than 64
    BOOLEAN bValidate = false;
    const NTSTATUS status = CiValidateFileObject(
        imgfile,
        0,
        0,
        &signerPolicyInfo,
        &timestampingAuthorityPolicyInfo,
        &signingTime,
        digestBuffer,
        &digestSize,
        &digestIdentifier
    );

    KdPrint(("[IrpCertCheck] CiValidateFileObject returned 0x%08X\n", status));
    if (NT_SUCCESS(status))
    {
#if PRINT_DIGEST 
        CHAR digestTempBuffer[98] = { 0 };
        for (int i = 0; i < 32; i++)
        {
            digestTempBuffer[3 * i] = signerPolicyInfo.certChainInfo->ptrToCertChainMembers->digestBuffer[i] >> 4;
            digestTempBuffer[3 * i + 1] = signerPolicyInfo.certChainInfo->ptrToCertChainMembers->digestBuffer[i] & 0xf;
            digestTempBuffer[3 * i + 2] = ' ';
        }
        for (int i = 0; i < 96; i++)
        {
            digestTempBuffer[i] = HexToChar(digestTempBuffer[i]);
        }
        KdPrint("¡iIrpCertCheck¡j", "Signer certificate:\n digest algorithm - 0x%x\n digest size - %d\r\n digest - %s\n",
            digestIdentifier, digestSize, digestTempBuffer);
#endif
        if (RtlCompareMemory(signerPolicyInfo.CertChain->PtrToCertChainMembers->DigestBuffer, digest, digest_size) == digest_size)
        {
            bValidate = true;
        }
    }
    CiFreePolicyInfo(&signerPolicyInfo);
    CiFreePolicyInfo(&timestampingAuthorityPolicyInfo);
    return bValidate;
}