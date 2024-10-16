#include <wdm.h>
#include <minwindef.h>
#include "FileSignatureCheck.h"
#include "Utils.h"

//Roy Note:
//CiXXXX APIs in ci.dll is used for kernel authenticode and driver signature verify.
//ELAM also use it. This is only way to support digital signature checking without 
//additional librarys in windows kernel.


/**
*  Given a file digest and signature of a file, verify the signature and provide information regarding
*  the certificates that was used for signing (the entire certificate chain)
*
*  @note  the function allocates a buffer from the paged pool --> can be used only where IRQL < DISPATCH_LEVEL
*
*  @param  DigestBuffer - buffer containing the digest
*
*  @param  DigestSize - size of the digest, e.g. 0x20 for SHA256, 0x14 for SHA1
*
*  @param  DigestAlgorithm - digest algorithm identifier, e.g. 0x800c for SHA256, 0x8004 for SHA1
*
*  @param  winCert - pointer to the start of the security directory
*
*  @param  sizeOfSecurityDirectory - size the security directory
*
*  @param  SignerPolicy[out] - PolicyInfo containing information about the signer certificate chain
*
*  @param  SigningTime[out] - when the file was signed (FILETIME format)
*
*  @param  TimeAuthPolicy[out] - PolicyInfo containing information about the timestamping
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

/**
*  Resets a PolicyInfo struct - frees the dynamically allocated buffer in PolicyInfo (certChainInfo) if not null.
*  Zeros the entire PolicyInfo struct.
*
*  @param  policyInfo - the struct to reset.
*
*  @return  the struct which was reset.
*/
extern "C" __declspec(dllimport) PVOID _stdcall CiFreePolicyInfo(PPOLICY_INFO policyInfo);
//extern CI_FREE_POLICY_INFO CiFreePolicyInfo;

/**
*  Given a file object, verify the signature and provide information regarding
*  the certificates that was used for signing (the entire certificate chain)
*
*  @note  the function allocates memory from the paged pool --> can be used only where IRQL < DISPATCH_LEVEL
*
*  @param  fileObject[in] - fileObject of the PE in question
*
*  @param  Reserved1[in] - unknown, needs to be reversed. 0 is a valid value.
*
*  @param  Reserved2[in] - unknown, needs to be reversed. 0 is a valid value.
*
*  @param  SignerPolicy[out] - PolicyInfo containing information about the signer certificate chain
*
*  @param  SigningTime[out] - when the file was signed
*
*  @param  TimeAuthPolicy[out] - PolicyInfo containing information about the timestamping
*          authority (TSA) certificate chain
*
*  @param  DigestBuffer[out] - buffer to be filled with the digest, must be at least 64 bytes
*
*  @param  DigestSize[inout] - size of the digest. Must be at leat 64 and will be changed by the function to
*                              reflect the actual digest length.
*
*  @param  DigestAlgorithm[out] - digest algorithm identifier, e.g. 0x800c for SHA256, 0x8004 for SHA1
*
*  @return  0 if the file digest in the signature matches the given digest and the signer cetificate is verified.
*           Various error values otherwise, for example:
*           STATUS_INVALID_IMAGE_HASH - the digest does not match the digest in the signature
*           STATUS_IMAGE_CERT_REVOKED - the certificate used for signing the file is revoked
*           STATUS_IMAGE_CERT_EXPIRED - the certificate used for signing the file has expired
*/
extern "C" __declspec(dllimport) NTSTATUS _stdcall CiValidateFileObject(
    struct _FILE_OBJECT* fileObject,
    int Reserved1,              //must be 0
    int Reserved2,              //must be 0
    PPOLICY_INFO SignerPolicy,
    PPOLICY_INFO TimeAuthPolicy,
    LARGE_INTEGER * SigningTime,
    UCHAR * DigestBuffer,
    UINT32 * DigestSize,
    UINT32 * DigestAlgorithm);      //0x8004==SHA1, 0x800C==SHA256

static __inline const char* ParseDigestAlgorithmName(int algorithm_id)
{
    switch(algorithm_id)
    {
    case SHA1_IDENTIFIER:
        return "SHA1";
    case SHA256_IDENTIFIER:
        return "SHA256";
    }

    return "UNKNOWN";
}

void PrintCertInfo(PFILE_OBJECT imgfile)
{
    POLICY_INFO signer_policy = { 0 };
    POLICY_INFO time_auth_policy = { 0 };   //timestamp authority policy
    LARGE_INTEGER sign_time = { 0 };
    UINT32 digest_size = 64;                    //must be greater than 64
    UINT32 algorithm = 0;                   //0x8004==SHA1, 0x800C==SHA256
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
        &algorithm);

    if (NT_SUCCESS(status))
    {
        PrintKdMsg("[Roy] CiValidateFileObject succeed!\n");
        PrintKdMsg("[Roy] verification status:0x%08X, digest algorithm:0x%X(%s), digest size:%d\n",
            signer_policy.VerificationStatus, algorithm, 
            ParseDigestAlgorithmName(algorithm), digest_size);

        if(nullptr!= signer_policy.CertChain)
        {
            for(int i=0; i< signer_policy.CertChain->NumberOfCertChainMembers; i++)
            {
                char issuer[64] = {0};
                char subject[64] = {0};
                RtlCopyMemory(issuer, signer_policy.CertChain->PtrToCertChainMembers[i].IssuerName.NamePtr, signer_policy.CertChain->PtrToCertChainMembers[i].IssuerName.NameLen);
                RtlCopyMemory(subject, signer_policy.CertChain->PtrToCertChainMembers[i].SubjectName.NamePtr, signer_policy.CertChain->PtrToCertChainMembers[i].SubjectName.NameLen);
                PrintKdMsg("[Roy] Issuer=%s, SubjectName=%s\n", issuer, subject);

                PrintKdMsg("[Roy] digest buffer:");
                for(UINT32 j=0; j< digest_size; j++)
                {
                    PrintKdMsg("%02X", signer_policy.CertChain->PtrToCertChainMembers[i].DigestBuffer[j]);
                }
                PrintKdMsg(("\n"));
            }
            PrintKdMsg(("\n"));
        }
    }
    else
        PrintKdMsg("[Roy] CiValidateFileObject failed => 0x%08X\n", status);

    CiFreePolicyInfo(&signer_policy);
    CiFreePolicyInfo(&time_auth_policy);
}

BOOLEAN IsProcessFileSignatureValid(
    PFILE_OBJECT imgfile,
    UINT32 algorithm,
    const BYTE digest[],
    UINT32 digest_size)
{
    BOOLEAN ret = FALSE;
    POLICY_INFO signer_policy = { 0 };
    POLICY_INFO time_auth_policy = { 0 };   //timestamp authority policy
    LARGE_INTEGER sign_time = { 0 };
    UINT32 ret_size = 0;                    //must be greater than 64
    UINT32 ret_algo = 0;                   //0x8004==SHA1, 0x800C==SHA256
    UCHAR buffer[64] = { 0 };          //must be greater than 64
    NTSTATUS status = CiValidateFileObject(
        imgfile,
        0,
        0,
        &signer_policy,
        &time_auth_policy,
        &sign_time,
        buffer,
        &ret_size,
        &ret_algo);
    PrintKdMsg("[Roy] try to validate File(%p) sinature: desired algorithm=%s, desired digest size=%d\n",
        imgfile, algorithm, digest);

    if (NT_SUCCESS(status))
    {
        PrintKdMsg("[Roy] CiValidateFileObject succeed!\n");
        PrintKdMsg("[Roy] verification status:0x%08X, returned info: algorithm=0x%X(%s), digest size=%d\n",
            signer_policy.VerificationStatus, ret_algo, 
            ParseDigestAlgorithmName(ret_algo), ret_size);

        if(ret_algo == algorithm && 
            ret_size == digest_size &&
            0 == RtlCompareMemory(buffer, digest, digest_size))
            ret = TRUE;
    }
    else
        PrintKdMsg("[Roy] CiValidateFileObject failed => 0x%08X\n", status);

    CiFreePolicyInfo(&signer_policy);
    CiFreePolicyInfo(&time_auth_policy);

    if(ret)
    {
        PrintKdMsg("[Roy] Digest verify matched!\n");
    }
    else
    {
        PrintKdMsg("[Roy] Signature digest not matched....T_T \n");
    }

    return ret;
}

#if 0
BOOLEAN ValidateFileUsingCiValidateFileObject(
    PFILE_OBJECT imgfile, 
    const BYTE digest[], 
    UINT32 algorithm,
    UINT32 digest_size)
{
    KdPrint(("[Roy] Validating file using CiValidateFileObject...\n"));
    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    POLICY_INFO signerPolicyInfo = { 0 };
    POLICY_INFO timestampingAuthorityPolicyInfo = { 0 };
    LARGE_INTEGER signingTime = { 0 };
    UINT32 digestSize = 64;            //must be greater than 64
    UINT32 digestIdentifier = 0;
    BYTE digestBuffer[64] = { 0 };  //must be more than 64 elements
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

    PrintKdMsg("[Roy]  CiValidateFileObject returned 0x%08X\n", status);
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
#endif
