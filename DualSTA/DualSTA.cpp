#include <windows.h>
#include <wlanapi.h>
#include <rpc.h>
#include <stdio.h>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "Rpcrt4.lib")

DWORD SetDualStaConnectivity(int enable)
{
    HANDLE hClient = NULL;
    DWORD negotiatedVersion = 0;
    DWORD result = ERROR_SUCCESS;

    // Declare all variables *before* any control flow
    PWLAN_INTERFACE_INFO_LIST ifaceList = NULL;
    PVOID secIfListRaw = NULL;
    PWLAN_INTERFACE_INFO_LIST secIfList = NULL;
    DWORD secIfListSize = 0;

    PVOID syncStateRaw = NULL;
    DWORD syncStateSize = 0;
    BOOL currentSyncState = FALSE;

    GUID primaryGuid = {};
    GUID secondaryGuid = {};

    BOOL newState = FALSE;

    //
    // Step 1: Open WLAN handle
    //
    result = WlanOpenHandle(2, NULL, &negotiatedVersion, &hClient);
    if (result != ERROR_SUCCESS)
    {
        printf("WlanOpenHandle failed: %lu\n", result);
        goto Cleanup;
    }

    //
    // Step 2: Enumerate primary interfaces
    //
    result = WlanEnumInterfaces(hClient, NULL, &ifaceList);
    if (result != ERROR_SUCCESS)
    {
        printf("WlanEnumInterfaces failed: %lu\n", result);
        goto Cleanup;
    }

    if (ifaceList->dwNumberOfItems == 0)
    {
        printf("No Wi-Fi interfaces found.\n");
        result = ERROR_NOT_FOUND;
        goto Cleanup;
    }

    primaryGuid = ifaceList->InterfaceInfo[0].InterfaceGuid;

    printf("Primary interface: %ws\n",
        ifaceList->InterfaceInfo[0].strInterfaceDescription);

    //
    // Step 3: Get secondary STA interfaces
    //
    result = WlanQueryInterface(
        hClient,
        &primaryGuid,
        wlan_intf_opcode_secondary_sta_interfaces,
        NULL,
        &secIfListSize,
        &secIfListRaw,
        NULL
    );

    if (result != ERROR_SUCCESS)
    {
        printf("This adapter does NOT support Secondary STA. Error: %lu\n", result);
        goto Cleanup;
    }

    secIfList = (PWLAN_INTERFACE_INFO_LIST)secIfListRaw;

    if (secIfList->dwNumberOfItems == 0)
    {
        printf("No secondary STA interfaces found.\n");
        result = ERROR_NOT_FOUND;
        goto Cleanup;
    }

    secondaryGuid = secIfList->InterfaceInfo[0].InterfaceGuid;

    printf("Secondary STA interface: %ws\n",
        secIfList->InterfaceInfo[0].strInterfaceDescription);

    //
    // Step 4: Query current sync setting
    //
    result = WlanQueryInterface(
        hClient,
        &secondaryGuid,
        wlan_intf_opcode_secondary_sta_synchronized_connections,
        NULL,
        &syncStateSize,
        &syncStateRaw,
        NULL
    );

    if (result != ERROR_SUCCESS)
    {
        printf("Cannot query sync setting (driver may not support it). Error: %lu\n", result);
        goto Cleanup;
    }

    currentSyncState = *(BOOL*)syncStateRaw;
    printf("Current synchronized-connections value: %d\n", currentSyncState);

    //
    // Step 5: Set new sync state
    //
    newState = enable ? TRUE : FALSE;

    result = WlanSetInterface(
        hClient,
        &secondaryGuid,
        wlan_intf_opcode_secondary_sta_synchronized_connections,
        sizeof(BOOL),
        &newState,
        NULL
    );

    if (result == ERROR_SUCCESS)
        printf("Successfully set Dual-STA synchronized-connections to %d\n", newState);
    else
        printf("Failed to set synchronized-connections. Error: %lu\n", result);

Cleanup:
    if (syncStateRaw) WlanFreeMemory(syncStateRaw);
    if (secIfListRaw) WlanFreeMemory(secIfListRaw);
    if (ifaceList) WlanFreeMemory(ifaceList);
    if (hClient) WlanCloseHandle(hClient, NULL);

    return result;
}

int main()
{
    printf("Enabling Dual-STA synchronized connections...\n");

    DWORD result = SetDualStaConnectivity(1);

    printf("Result: %lu\n", result);
    printf("Waiting forever. Press CTRL + C to exit.\n");

    Sleep(INFINITE);
    return 0;
}
